#include "include.h"
#include "mupdf/pdf.h"

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

	data = t.data;
	t.data = nullptr;

	m_pages = std::move(t.m_pages);
}


MuPDFHandler::PDF& MuPDFHandler::PDF::operator=(PDF&& t) noexcept {
	if (this != &t) {
		m_doc = t.m_doc;
		t.m_doc = nullptr;

		m_ctx = t.m_ctx;
		t.m_ctx = nullptr;

		data = t.data;
		t.data = nullptr;

		m_pages = std::move(t.m_pages);
	}
	return *this;
}

MuPDFHandler::PDF::~PDF() {
	if (m_ctx == nullptr)
		return;
	for (size_t i = 0; i < m_pages.size(); i++) {
		fz_drop_page(m_ctx, m_pages.at(i));
	}
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

void do_multithread_rendering(PDFHandler* handerl, Direct2DRenderer* renderer, fz_context* ctx, fz_display_list* list, Renderer::Rectangle<float> source, Renderer::Rectangle<float> dest, float dpi) {
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

void MuPDFHandler::PDF::multithreaded_get_bitmap(Direct2DRenderer* renderer, size_t page, Renderer::Rectangle<float> source, Renderer::Rectangle<float> dest, float dpi, PDFHandler* pdfhandler) const {
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
		std::thread(do_multithread_rendering, pdfhandler, renderer, m_ctx, list, source_recs.at(i), destin_recs.at(i), dpi).detach();
	}
}

size_t MuPDFHandler::PDF::get_page_count() const { 
	return fz_count_pages(m_ctx, m_doc);
}

Renderer::Rectangle<float> MuPDFHandler::PDF::get_page_size(size_t page, float dpi) const {
	Renderer::Rectangle<float> rect;
	rect = fz_bound_page(m_ctx, get_page(page));
	return Renderer::Rectangle<float>(0, 0, rect.width * (dpi/ MUPDF_DEFAULT_DPI), rect.height * (dpi/ MUPDF_DEFAULT_DPI));
}
