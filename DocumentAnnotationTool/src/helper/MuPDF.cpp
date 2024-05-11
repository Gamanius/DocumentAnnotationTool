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

//void MuPDFHandler::PDF::create_display_list() {
//	// for each rec create a display list
//	fz_display_list* list = nullptr;
//	fz_device* dev = nullptr;
//
//	m_display_lists = new std::vector<fz_display_list*>();
//
//	for (size_t i = 0; i < get_page_count(); i++) {
//		// get the page that will be rendered
//		auto p = get_page(i);
//		fz_try(m_ctx) {
//			// create a display list with all the draw calls and so on
//			list = fz_new_display_list(m_ctx, fz_bound_page(m_ctx, p)); 
//			dev = fz_new_list_device(m_ctx, list); 
//			// run the device
//			fz_run_page(m_ctx, p, dev, fz_identity, nullptr); 
//			// add list to array
//			m_display_lists->push_back(list); 
//		} fz_always(m_ctx) {
//			// flush the device
//			fz_close_device(m_ctx, dev);  
//			fz_drop_device(m_ctx, dev);  
//		} fz_catch(m_ctx) {
//			ASSERT(false, "Could not create display list");
//		}
//	}
//}

MuPDFHandler::PDF::PDF(fz_context* ctx, fz_document* doc) {
	m_ctx = ctx;
	m_doc = doc; 

	for (size_t i = 0; i < get_page_count(); i++) { 
		m_pages.push_back(fz_load_page(m_ctx, m_doc, i)); 
	}
}

MuPDFHandler::PDF::PDF(PDF&& t) noexcept {
	m_doc = t.m_doc;
	t.m_doc = nullptr;

	m_ctx = t.m_ctx;
	t.m_ctx = nullptr;

	m_pages = std::move(t.m_pages);

	data = t.data;
	t.data = nullptr;
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

	// we can now safely delete all the other used resources
	for (size_t i = 0; i < m_pages.size(); i++) {
		fz_drop_page(m_ctx, m_pages.at(i));
	}


	fz_drop_document(m_ctx, m_doc);
}

fz_page* MuPDFHandler::PDF::get_page(size_t page) const {
	return m_pages.at(page);
}

fz_context* MuPDFHandler::PDF::get_context() const {
	return m_ctx;
}

/*

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
*/

size_t MuPDFHandler::PDF::get_page_count() const { 
	return fz_count_pages(m_ctx, m_doc);
}

Renderer::Rectangle<float> MuPDFHandler::PDF::get_page_size(size_t page, float dpi) const {
	Renderer::Rectangle<float> rect;
	rect = fz_bound_page(m_ctx, get_page(page));
	return Renderer::Rectangle<float>(0, 0, rect.width * (dpi/ MUPDF_DEFAULT_DPI), rect.height * (dpi/ MUPDF_DEFAULT_DPI));
}
