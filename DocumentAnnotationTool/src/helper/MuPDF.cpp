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
	Logger::error("MuPDF: \"", message, "\"");
}

void MuPDFHandler::warning_callback(void* user, const char* message) {
	Logger::warn("MuPDF: \"", message, "\"");
}

MuPDFHandler::MuPDFHandler() { 
	m_locks.user = m_mutex; 
	m_locks.lock = lock_mutex; 
	m_locks.unlock = unlock_mutex;

	m_context = std::shared_ptr<ContextWrapper>(new ContextWrapper(fz_new_context(nullptr, &m_locks, FZ_STORE_UNLIMITED)));
	
	auto c = m_context->get_item();

	fz_set_error_callback(*c, error_callback, nullptr);
	fz_set_warning_callback(*c, warning_callback, nullptr);
	fz_register_document_handlers(*c);
}

std::optional<MuPDFHandler::PDF> MuPDFHandler::load_pdf(const std::wstring& path) {
	auto ctx = get_context();

	// load the pdf 
	auto file = FileHandler::open_file(path);
	ASSERT_RETURN_NULLOPT(file.has_value(), L"Could not open file ", path);

	auto stream = fz_open_memory(*ctx, file.value().data, file.value().size);
	auto doc = fz_open_document_with_stream(*ctx, ".pdf", stream);

	auto d = std::shared_ptr<DocumentWrapper>(new DocumentWrapper(m_context, doc));
	fz_drop_stream(*ctx, stream);

	ASSERT_RETURN_NULLOPT(doc != nullptr, L"Could not open document ", path); 

	auto pdf = PDF(m_context, d);
	pdf.data = file.value().data;
	file.value().data = nullptr;
	pdf.size = file.value().size;

	return std::move(pdf);
}

ThreadSafeContextWrapper MuPDFHandler::get_context() {
	return m_context->get_item();
}

MuPDFHandler::~MuPDFHandler() {}

MuPDFHandler::PDF::PDF(std::shared_ptr<ContextWrapper> ctx, std::shared_ptr<DocumentWrapper> doc) {
	m_ctx = ctx;
	m_document = doc; 

	// get context
	auto c = ctx->get_item();
	// and document
	auto d = doc->get_document();

	m_pagerec = std::shared_ptr<ThreadSafeVector<Renderer::Rectangle<float>>>
		(new ThreadSafeVector<Renderer::Rectangle<float>>(std::vector<Renderer::Rectangle<float>>()));

	sort_page_positions();
}

MuPDFHandler::PDF::PDF(PDF&& t) noexcept {
	m_document = std::move(t.m_document);

	m_ctx = std::move(t.m_ctx);

	data = t.data;
	t.data = nullptr;

	m_seperation_distance = t.m_seperation_distance;
	t.m_seperation_distance = 0;

	m_pagerec = std::move(t.m_pagerec);

}


MuPDFHandler::PDF& MuPDFHandler::PDF::operator=(PDF&& t) noexcept {
	// new c++ stuff?
	this->~PDF();
	new(this) PDF(std::move(t));
	return *this;
}

MuPDFHandler::PDF::~PDF() {
	// pages are going to be deleted by the wrapper
	// so there is nothing ot be deleted
}
ThreadSafeContextWrapper MuPDFHandler::PDF::get_context() const {
	return m_ctx->get_item();
}

ThreadSafeDocumentWrapper MuPDFHandler::PDF::get_document() const {
	return m_document->get_item();
}

std::shared_ptr<ThreadSafeVector<Renderer::Rectangle<float>>> MuPDFHandler::PDF::get_pagerec() {
	return m_pagerec;
}

PageWrapper MuPDFHandler::PDF::get_page(size_t page) {
	auto ctx = m_ctx->get_context();
	auto doc = m_document->get_item();
	return std::move(PageWrapper(get_context_wrapper(), fz_load_page(*ctx, *doc, (int)page))); 
}

PageWrapper MuPDFHandler::PDF::get_page(fz_document* doc, size_t page) {
	auto ctx = m_ctx->get_context();
	return std::move(PageWrapper(get_context_wrapper(), fz_load_page(*ctx, doc, (int)page)));
}


std::shared_ptr<ContextWrapper> MuPDFHandler::PDF::get_context_wrapper() const {
	return m_ctx;
}

size_t MuPDFHandler::PDF::get_page_count() const { 
	auto c = m_ctx->get_context();
	auto d = m_document->get_document();
	return fz_count_pages(*c, *d);
}

void MuPDFHandler::PDF::save_pdf(const std::wstring& path) {
	auto ctx = m_ctx->get_context();
	auto doc = m_document->get_document(); 

	fz_try(*ctx) {
		pdf_write_options opt = {0};
		opt.permissions = ~0;
		opt.do_compress_images = 0;
		opt.do_compress = 0;

		fz_buffer* buffer = fz_new_buffer(*ctx, 0); 
		fz_output* output = fz_new_output_with_buffer(*ctx, buffer);
		pdf_write_document(*ctx, reinterpret_cast<pdf_document*>(*doc), output, &opt);

		ASSERT_WIN(FileHandler::write_file(buffer->data, buffer->len, path), "Could not write pdf to file");

		fz_close_output(*ctx, output);
		fz_drop_output(*ctx, output);
		fz_drop_buffer(*ctx, buffer);
	} fz_catch(*ctx) {
		fz_report_error(*ctx);
	}
}

Renderer::Rectangle<float> MuPDFHandler::PDF::get_page_size(size_t page, float dpi) { 
	auto c = m_ctx->get_context();
	auto d = m_document->get_document();
	auto p = get_page(*d, page);

	Renderer::Rectangle<float> rect;
	rect = fz_bound_page(*c, p);
	return Renderer::Rectangle<float>(0, 0, rect.width * (dpi/ MUPDF_DEFAULT_DPI), rect.height * (dpi/ MUPDF_DEFAULT_DPI));
}

Renderer::Rectangle<float> MuPDFHandler::PDF::get_bounds() const {
	auto write = m_pagerec->get_read();

	auto d = write->at(0);
	float up = d.y, down = d.bottom(), left = d.x, right = d.right();

	for (size_t i = 0; i < write->size(); i++) {
		auto& r = write->at(i);
		left = min(left, r.x);
		up = min(up, r.y);
		right = max(right, r.right());
		down = max(down, r.bottom());
	}

	return Renderer::Rectangle<float>(left, up, right - left, down - up); 
} 


void MuPDFHandler::PDF::sort_page_positions() {
	auto dest = m_pagerec->get_write();
	dest->clear();
	double height = 0;
	for (size_t i = 0; i < get_page_count(); i++) {
		auto size = get_page_size(i);
		dest->push_back(Renderer::Rectangle<double>(0, height, size.width, size.height));
		height += get_page_size(i).height;
		height += m_seperation_distance;
	}
}