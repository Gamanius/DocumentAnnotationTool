#include "include.h"
#include "mupdf/pdf.h"
#include <sstream>
#include <exception>

void MuPDFHandler::lock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].lock();
}

void MuPDFHandler::unlock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].unlock();
}

void MuPDFHandler::error_callback(void* user, const char* message) {
	ASSERT(false, "Error in MuPDFHandler: " + std::string(message));
}

MuPDFHandler::MuPDFHandler() { 
	m_locks.user = m_mutex; 
	m_locks.lock = lock_mutex; 
	m_locks.unlock = unlock_mutex; 

	m_ctx = fz_new_context(nullptr, &m_locks, FZ_STORE_UNLIMITED);
	fz_set_error_callback(m_ctx, error_callback, nullptr); 
	fz_register_document_handlers(m_ctx);
}

std::optional<MuPDFHandler::PDF> MuPDFHandler::load_pdf(const std::wstring& path) {
	// load the pdf 
	auto file = FileHandler::open_file(path);
	ASSERT_RETURN_NULLOPT(file.has_value(), L"Could not open file " + path);

	auto stream = fz_open_memory(m_ctx, file.value().data, file.value().size);
	auto doc = fz_open_document_with_stream(m_ctx, ".pdf", stream);
	fz_drop_stream(m_ctx, stream);

	ASSERT_RETURN_NULLOPT(doc != nullptr, L"Could not open document " + path); 

	auto pdf = PDF(m_ctx, doc);
	pdf.data = file.value().data;
	file.value().data = nullptr;
	pdf.size = file.value().size;

	return std::move(pdf); 
}

MuPDFHandler::~MuPDFHandler() {
	fz_drop_context(m_ctx);
}

// PDF CODE //

void MuPDFHandler::PDF::create_display_list() {
	// for each rec create a display list
	fz_display_list* list = nullptr;
	fz_device* dev = nullptr;

	m_display_lists = new std::vector<fz_display_list*>();

	for (size_t i = 0; i < get_page_count(); i++) {
		// get the page that will be rendered
		auto p = get_page(i);
		fz_try(m_ctx) {
			// create a display list with all the draw calls and so on
			list = fz_new_display_list(m_ctx, fz_bound_page(m_ctx, p)); 
			dev = fz_new_list_device(m_ctx, list); 
			// run the device
			fz_run_page(m_ctx, p, dev, fz_identity, nullptr); 
			// add list to array
			m_display_lists->push_back(list); 
		} fz_always(m_ctx) {
			// flush the device
			fz_close_device(m_ctx, dev);  
			fz_drop_device(m_ctx, dev);  
		} fz_catch(m_ctx) {
			ASSERT(false, "Could not create display list");
		}
	}
}

MuPDFHandler::PDF::PDF(fz_context* ctx, fz_document* doc) {
	m_ctx = ctx;
	m_doc = doc; 

	for (size_t i = 0; i < get_page_count(); i++) { 
		m_pages.push_back(fz_load_page(m_ctx, m_doc, i)); 
	}

	create_display_list();
	// create the conditional variable
	m_thread_conditional = new std::condition_variable(); 
	m_queue_mutex = new std::mutex();
	m_threaded_render_queue = new std::deque<std::tuple<PDFRenderInfoStruct, HWND, std::vector<CachedBitmap*>>>();
	m_thread_should_die = new bool(false); 
	m_render_in_progress = new std::atomic<size_t>(0);
	m_amount_of_threads_running = new std::atomic<size_t>(0);
}

MuPDFHandler::PDF::PDF(PDF&& t) noexcept {
	m_doc = t.m_doc;
	t.m_doc = nullptr;

	m_ctx = t.m_ctx;
	t.m_ctx = nullptr;

	m_pages = std::move(t.m_pages);

	m_display_lists = t.m_display_lists;
	t.m_display_lists = nullptr;

	data = t.data;
	t.data = nullptr;

	m_amount_of_threads_running = t.m_amount_of_threads_running;
	t.m_amount_of_threads_running = nullptr;

	m_thread_conditional = t.m_thread_conditional;
	t.m_thread_conditional = nullptr;

	m_thread_should_die = t.m_thread_should_die;
	t.m_thread_should_die = nullptr;

	m_queue_mutex = t.m_queue_mutex;
	t.m_queue_mutex = nullptr;

	m_threaded_render_queue = t.m_threaded_render_queue;
	t.m_threaded_render_queue = nullptr;

	m_render_in_progress = t.m_render_in_progress;
	t.m_render_in_progress = 0;
}


MuPDFHandler::PDF& MuPDFHandler::PDF::operator=(PDF&& t) noexcept {
	// new c++ stuff?
	this->~PDF();
	new(this) PDF(std::move(t));
	return *this;
}

MuPDFHandler::PDF::~PDF() {
	if (m_ctx == nullptr)
		return;

	// firts cleanup thethreads
	stop_render_threads();

	if (m_thread_conditional != nullptr) {
		delete m_thread_conditional;
	}
	if (m_queue_mutex != nullptr) {
		delete m_queue_mutex;
	}

	delete m_thread_should_die;
	delete m_amount_of_threads_running;
	delete m_threaded_render_queue;
	delete m_render_in_progress;

	// we can now safely delete all the other used resources
	for (size_t i = 0; i < m_pages.size(); i++) {
		fz_drop_page(m_ctx, m_pages.at(i));
	}

	for (size_t i = 0; i < m_display_lists->size(); i++) {
		fz_drop_display_list(m_ctx, m_display_lists->at(i));
	}
	delete m_display_lists;

	fz_drop_document(m_ctx, m_doc);
}

fz_page* MuPDFHandler::PDF::get_page(size_t page) const {
	return m_pages.at(page);
}

Direct2DRenderer::BitmapObject MuPDFHandler::PDF::get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi) const {
	fz_matrix ctm;
	auto size = get_page_size(page, dpi); 
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));

	fz_pixmap* pixmap     = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	fz_try(m_ctx) {
		 pixmap     = fz_new_pixmap(m_ctx, fz_device_rgb(m_ctx), size.width, size.height, nullptr, 1);
		 fz_clear_pixmap_with_value(m_ctx, pixmap, 0xff); // for the white background
		 drawdevice = fz_new_draw_device(m_ctx, fz_identity, pixmap); 
		 fz_run_page(m_ctx, get_page(page), drawdevice, ctm, nullptr);
		 fz_close_device(m_ctx, drawdevice);
		 obj = renderer.create_bitmap((byte*)pixmap->samples, size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
	} fz_always(m_ctx) {
		fz_drop_device(m_ctx, drawdevice);
		fz_drop_pixmap(m_ctx, pixmap);
	} fz_catch(m_ctx) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj);
}

Direct2DRenderer::BitmapObject MuPDFHandler::PDF::get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<unsigned int> rec) const {
	fz_matrix ctm; 
	auto size = get_page_size(page);
	ctm = fz_scale((rec.width / size.width), (rec.height / size.height)); 

	// this is to calculate the size of the pixmap
	auto bbox = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, size.width, size.height), ctm)); 

	fz_pixmap* pixmap = nullptr; 
	fz_device* drawdevice = nullptr; 
	Direct2DRenderer::BitmapObject obj; 

	fz_try(m_ctx) {
		pixmap = fz_new_pixmap_with_bbox(m_ctx, fz_device_rgb(m_ctx), bbox, nullptr, 1);
		fz_clear_pixmap_with_value(m_ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(m_ctx, fz_identity, pixmap);
		fz_run_page(m_ctx, get_page(page), drawdevice, ctm, nullptr);
		fz_close_device(m_ctx, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, rec, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
	} fz_always(m_ctx) {
		fz_drop_device(m_ctx, drawdevice);
		fz_drop_pixmap(m_ctx, pixmap);
	} fz_catch(m_ctx) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj);
}

Direct2DRenderer::BitmapObject MuPDFHandler::PDF::get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<float> source, float dpi) const {
	fz_matrix ctm;
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));
	auto transform = fz_translate(-source.x, -source.y);
	// this is to calculate the size of the pixmap using the source
	auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, source.width, source.height), ctm));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	fz_try(m_ctx) {
		pixmap = fz_new_pixmap_with_bbox(m_ctx, fz_device_rgb(m_ctx), pixmap_size, nullptr, 1); 
		fz_clear_pixmap_with_value(m_ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(m_ctx, fz_identity, pixmap);
		fz_run_page(m_ctx, get_page(page), drawdevice, fz_concat(transform, ctm), nullptr);
		fz_close_device(m_ctx, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, dpi); // default dpi of the pixmap
	} fz_always(m_ctx) {
		fz_drop_device(m_ctx, drawdevice);
		fz_drop_pixmap(m_ctx, pixmap);
	} fz_catch(m_ctx) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj); 
}

void do_multithread_rendering(Direct2DRenderer* renderer, fz_context* ctx, fz_display_list* list, Renderer::Rectangle<float> source, Renderer::Rectangle<float> dest, float dpi) {
	// clone the context
	fz_context* cloned_ctx = fz_clone_context(ctx);

	fz_matrix ctm;
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));
	auto transform = fz_translate(-source.x, -source.y);
	// this is to calculate the size of the pixmap using the source
	auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, source.width, source.height), ctm)); 

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	fz_try(cloned_ctx) {
		// create new pixmap
		pixmap = fz_new_pixmap_with_bbox(cloned_ctx, fz_device_rgb(cloned_ctx), pixmap_size, nullptr, 1);
		fz_clear_pixmap_with_value(cloned_ctx, pixmap, 0xff); // for the white background
		// create draw device
		drawdevice = fz_new_draw_device(cloned_ctx, fz_concat(transform, ctm), pixmap); 
		// render to draw device
		fz_run_display_list(cloned_ctx, list, drawdevice, fz_identity, source, nullptr);  
		fz_close_device(cloned_ctx, drawdevice);
		// create the bitmap
		obj = renderer->create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, dpi); // default dpi of the pixmap
		// now call begin thre drawing
		renderer->begin_draw();
		renderer->set_current_transform_active();
		renderer->draw_bitmap(obj, dest, Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
		renderer->end_draw();
	} fz_always(cloned_ctx) {
		// drop all devices
		fz_drop_device(cloned_ctx, drawdevice);
		fz_drop_pixmap(cloned_ctx, pixmap);
		fz_drop_display_list(cloned_ctx, list); 
	} fz_catch(cloned_ctx) {
		return;
	}

	fz_drop_context(cloned_ctx); 
}

void do_multithread_rendering_with_windowcallback(Direct2DRenderer* renderer, fz_context* ctx, fz_display_list* list, Renderer::Rectangle<float> source, Renderer::Rectangle<float> dest, float dpi) {
	// clone the context
	fz_context* cloned_ctx = fz_clone_context(ctx);

	fz_matrix ctm;
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));
	auto transform = fz_translate(-source.x, -source.y);
	// this is to calculate the size of the pixmap using the source
	auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, source.width, source.height), ctm));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	fz_try(cloned_ctx) {
		// create new pixmap
		pixmap = fz_new_pixmap_with_bbox(cloned_ctx, fz_device_rgb(cloned_ctx), pixmap_size, nullptr, 1);
		fz_clear_pixmap_with_value(cloned_ctx, pixmap, 0xff); // for the white background
		// create draw device
		drawdevice = fz_new_draw_device(cloned_ctx, fz_concat(transform, ctm), pixmap);
		// render to draw device
		fz_run_display_list(cloned_ctx, list, drawdevice, fz_identity, source, nullptr);
		fz_close_device(cloned_ctx, drawdevice);
		// create the bitmap
		obj = renderer->create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, dpi); // default dpi of the pixmap

	} fz_always(cloned_ctx) {
		// drop all devices
		fz_drop_device(cloned_ctx, drawdevice);
		fz_drop_pixmap(cloned_ctx, pixmap);
		fz_drop_display_list(cloned_ctx, list);
	} fz_catch(cloned_ctx) {
		return;
	}

	fz_drop_context(cloned_ctx);
}

void MuPDFHandler::PDF::multithreaded_get_bitmap(Direct2DRenderer* renderer, size_t page, Renderer::Rectangle<float> source, Renderer::Rectangle<float> dest, float dpi) const {
	float halfwidth  = source.width / 2.0f;
	float halfheight = source.height / 2.0f;

	// create the 4 recs
	Renderer::Rectangle<float> r1 = { source.x, source.y, halfwidth, halfheight };
	Renderer::Rectangle<float> r2 = { source.x + halfwidth, source.y, halfwidth, halfheight };
	Renderer::Rectangle<float> r3 = { source.x, source.y + halfheight, halfwidth, halfheight };
	Renderer::Rectangle<float> r4 = { source.x + halfwidth, source.y + halfheight, halfwidth, halfheight };

	// calculate the destination rectangle
	halfwidth  = dest.width / 2.0f;
	halfheight = dest.height / 2.0f;
	Renderer::Rectangle<float> d1 = { dest.x, dest.y, halfwidth, halfheight };
	Renderer::Rectangle<float> d2 = { dest.x + halfwidth, dest.y, halfwidth, halfheight };
	Renderer::Rectangle<float> d3 = { dest.x, dest.y + halfheight, halfwidth, halfheight };
	Renderer::Rectangle<float> d4 = { dest.x + halfwidth, dest.y + halfheight, halfwidth, halfheight };

	// create the source and destination rectangles

	std::array<Renderer::Rectangle<float>, 4> source_recs = { r1, r2, r3, r4 };
	std::array<Renderer::Rectangle<float>, 4> destin_recs = { d1, d2, d3, d4 };

	// for each rec create a display list
	fz_display_list* list = nullptr;
	fz_device* dev		  = nullptr;
	// get the page that will be rendered
	auto p = get_page(page);  
	for (size_t i = 0; i < source_recs.size(); i++) {
		fz_try(m_ctx) {
			// create a display list with all the draw calls and so on
			list = fz_new_display_list(m_ctx, source_recs.at(i)); 
			dev = fz_new_list_device(m_ctx, list);
			// run the device
			fz_run_page(m_ctx, p, dev, fz_identity, nullptr); 
		} fz_always(m_ctx) {
			// flush the device
			fz_close_device(m_ctx, dev);
			fz_drop_device(m_ctx, dev);
		} fz_catch(m_ctx) {
			return;
		}
		// if everything worked we can now create a thread and render the display list
		std::thread(do_multithread_rendering, renderer, m_ctx, list, source_recs.at(i), destin_recs.at(i), dpi).detach();
	}
}


typedef struct {
	unsigned int cmd : 5;
	unsigned int size : 9;
	unsigned int rect : 1;
	unsigned int path : 1;
	unsigned int cs : 3;
	unsigned int color : 1;
	unsigned int alpha : 2;
	unsigned int ctm : 3;
	unsigned int stroke : 1;
	unsigned int flags : 6;
} fz_display_node;


struct fz_display_list {
	fz_storable storable;
	fz_display_node* list;
	fz_rect mediabox;
	size_t max;
	size_t len;
};

void do_multihread_rendering(fz_context* ctx, std::condition_variable* cv, std::mutex* m, 
	std::deque<std::tuple<MuPDFHandler::PDF::PDFRenderInfoStruct, HWND, std::vector<CachedBitmap*>>>* render_queue,
	bool* die, std::vector <fz_display_list*>* displaylist, std::deque<CachedBitmap>* globalcachedbitmaps, 
	std::atomic<size_t>* amount_of_threads, std::atomic<size_t>* renderinprogress) {

	fz_context* cloned_ctx = fz_clone_context(ctx);

	// Copy all lists
	// this is most likely not safe AT ALL
	std::vector<fz_display_list*> cloned_lists; 
	for (size_t i = 0; i < displaylist->size(); i++) {
		fz_display_list* ls = fz_new_display_list(cloned_ctx, displaylist->at(i)->mediabox);
		ls->len = displaylist->at(i)->len;
		ls->max = displaylist->at(i)->max;
		ls->list = new fz_display_node[ls->max]; 
		memcpy(ls->list, displaylist->at(i)->list, displaylist->at(i)->len * sizeof(fz_display_node));

		cloned_lists.push_back(ls);
	}

	while (!(*die)) {
		// lock to get access to the deque and wait for the conditional variable
		std::unique_lock<std::mutex> lock(*m);
		cv->wait(lock, [&] { return render_queue->size() != 0; });
		// first check if cleanup is neccesesary
		if (*die) { 
			break;
		}
		// If notified remove the first element and render it
		MuPDFHandler::PDF::PDFRenderInfoStruct pdfrenderinfo;
		HWND window; 
		std::vector<CachedBitmap*> cached_bitmaps;
		if (!render_queue->empty()) { 
			auto temp = render_queue->front();

			pdfrenderinfo = std::get<0>(temp);
			window = std::get<1>(temp);
			cached_bitmaps = std::get<2>(temp);  

			(*renderinprogress)++;
			render_queue->pop_front(); 
		}
		else {
			lock.unlock(); 
			continue;
		}

		// TODO: copy display list and ctx
		// maybe do it in the begining of the thread life?
		auto& list = cloned_lists.at(pdfrenderinfo.page);
		
		// we can unlock now since we are done with everything
		lock.unlock();

		fz_matrix ctm;
		ctm = fz_scale((pdfrenderinfo.dpi / MUPDF_DEFAULT_DPI), (pdfrenderinfo.dpi / MUPDF_DEFAULT_DPI));
		auto transform = fz_translate(-pdfrenderinfo.source.x, -pdfrenderinfo.source.y);
		// this is to calculate the size of the pixmap using the source
		auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, pdfrenderinfo.source.width, pdfrenderinfo.source.height), ctm));

		fz_pixmap* pixmap = nullptr;
		fz_device* drawdevice = nullptr;
		Direct2DRenderer::BitmapObject obj;

		fz_try(cloned_ctx) {
			// create new pixmap
			pixmap = fz_new_pixmap_with_bbox(cloned_ctx, fz_device_rgb(cloned_ctx), pixmap_size, nullptr, 1);
			fz_clear_pixmap_with_value(cloned_ctx, pixmap, 0xff); // for the white background
			// create draw device
			drawdevice = fz_new_draw_device(cloned_ctx, fz_concat(transform, ctm), pixmap);
			// render to draw device
			fz_run_display_list(cloned_ctx, list, drawdevice, fz_identity, pdfrenderinfo.source, nullptr);
			fz_close_device(cloned_ctx, drawdevice);
			// create the bitmap
			obj = pdfrenderinfo.renderer->create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, pdfrenderinfo.dpi); // default dpi of the pixmap

			// we can call the window and lock the mutex since we are modifying stuff again
			lock.lock();
			auto myid = std::this_thread::get_id();
			std::stringstream ss; 
			ss << myid;
			std::string mystring = ss.str();
			Logger::log("Thread: " + mystring + " is rendering page: " + std::to_string(pdfrenderinfo.page) + " with dpi: " + std::to_string(pdfrenderinfo.dpi) + " and source: " + std::to_string(pdfrenderinfo.source.x) + " " + std::to_string(pdfrenderinfo.source.y) + " " + std::to_string(pdfrenderinfo.source.width) + " " + std::to_string(pdfrenderinfo.source.height));
			Logger::print_to_debug();
			auto new_cachedbitmap = CachedBitmap(std::move(obj), pdfrenderinfo.dest, pdfrenderinfo.dpi, 1);
			cached_bitmaps.push_back(&new_cachedbitmap);
			// unlock to avoid deadlock (If this thread got the lock first and the main thread is waiting on the lock, not unlocking will
			// lead to a deadlock)
			lock.unlock();
			SendMessage(window, WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&cached_bitmaps));
			lock.lock();
			// we can now decreament the used counter
			for (size_t i = 0; i < cached_bitmaps.size(); i++) {
				if (cached_bitmaps.at(i)->used != 0)
					cached_bitmaps.at(i)->used--;
			}
			// now add the new cached bitmap to the other ones
			globalcachedbitmaps->push_back(std::move(new_cachedbitmap));
			lock.unlock();
		} fz_always(cloned_ctx) {
			// drop all devices
			fz_drop_device(cloned_ctx, drawdevice);
			fz_drop_pixmap(cloned_ctx, pixmap);
		} fz_catch(cloned_ctx) {
			return;
		}
		(*renderinprogress)--;
	}
	// delete all the list elements
	for (size_t i = 0; i < cloned_lists.size(); i++) {
		// also not safe and probably not intended
		delete[] cloned_lists.at(i)->list;
		cloned_lists.at(i)->list = nullptr;
		cloned_lists.at(i)->len = 0;
		cloned_lists.at(i)->max = 0;
		fz_drop_display_list(cloned_ctx, cloned_lists.at(i));
	}
	(*amount_of_threads)--;
	cv->notify_all();
	fz_drop_context(cloned_ctx); 
}

void MuPDFHandler::PDF::multihreaded_get_bitmap(PDFRenderInfoStruct pdfrenderinfo, HWND window_callback, std::vector<CachedBitmap*> cached_bitmap) {
	if (*m_amount_of_threads_running == 0) {
		// dont do anything if no threads exists
		return;
	}
	// aquire lock to be able to add something to the queue
	std::unique_lock<std::mutex> lock_mutex(*m_queue_mutex);
	// add the render info to the queue
	m_threaded_render_queue->push_back(std::make_tuple(pdfrenderinfo, window_callback, cached_bitmap));  

	// now notify all the threads
	m_thread_conditional->notify_all();
}



void MuPDFHandler::PDF::create_render_threads(size_t amount, std::deque<CachedBitmap>* globalcachedbitmaps) {
	*m_amount_of_threads_running = amount;
	// create the threads
	for (size_t i = 0; i < *m_amount_of_threads_running; i++) {
		std::thread(do_multihread_rendering, m_ctx, m_thread_conditional, m_queue_mutex, m_threaded_render_queue,
			m_thread_should_die, m_display_lists, globalcachedbitmaps, m_amount_of_threads_running, m_render_in_progress).detach(); 
	}
}

void MuPDFHandler::PDF::stop_render_threads(bool blocking) {
	if (*m_render_in_progress != 0 and blocking)
		std::terminate();
	*m_thread_should_die = true;
	// check how many threads are alive
	if (*m_amount_of_threads_running == 0) {
		return;
	}
	// put dummy data in the queue
	m_threaded_render_queue->push_back(std::make_tuple(PDFRenderInfoStruct(), nullptr, std::vector<CachedBitmap*>())); 
	// notify all threads 
	m_thread_conditional->notify_all();
	// aquire conditional variable and wait
	if (blocking) {
		std::unique_lock<std::mutex> lock(*m_queue_mutex);
		m_thread_conditional->wait(lock, [this] { return *m_amount_of_threads_running == 0; });
	}
}

bool MuPDFHandler::PDF::multithreaded_is_rendering_in_progress() const {
	return *m_render_in_progress != 0;
}

size_t MuPDFHandler::PDF::get_page_count() const { 
	return fz_count_pages(m_ctx, m_doc);
}

Renderer::Rectangle<float> MuPDFHandler::PDF::get_page_size(size_t page, float dpi) const {
	Renderer::Rectangle<float> rect;
	rect = fz_bound_page(m_ctx, get_page(page));
	return Renderer::Rectangle<float>(0, 0, rect.width * (dpi/ MUPDF_DEFAULT_DPI), rect.height * (dpi/ MUPDF_DEFAULT_DPI));
}
