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
	auto size = get_page_size(page, dpi);
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
		obj = renderer.create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
	} fz_always(m_ctx) {
		fz_drop_device(m_ctx, drawdevice);
		fz_drop_pixmap(m_ctx, pixmap);
	} fz_catch(m_ctx) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj); 
}

size_t MuPDFHandler::PDF::get_page_count() const { 
	return fz_count_pages(m_ctx, m_doc);
}

Renderer::Rectangle<float> MuPDFHandler::PDF::get_page_size(size_t page, float dpi) const {
	Renderer::Rectangle<float> rect;
	rect = fz_bound_page(m_ctx, get_page(page));
	return Renderer::Rectangle<float>(0, 0, rect.width * (dpi/ MUPDF_DEFAULT_DPI), rect.height * (dpi/ MUPDF_DEFAULT_DPI));
}
