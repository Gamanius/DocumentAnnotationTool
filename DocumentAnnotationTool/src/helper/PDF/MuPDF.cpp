#include "mupdf/pdf.h"
#include "MuPDF.h"
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
	if (!file.has_value()) {
		Logger::warn(L"Could not open file: ", path);
		return std::nullopt;
	};

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

MuPDFHandler::~MuPDFHandler() {

}

MuPDFHandler::PDF::PDF(std::shared_ptr<ContextWrapper> ctx, std::shared_ptr<DocumentWrapper> doc) {
	m_ctx = ctx;
	m_document = doc; 

	// get context
	auto c = ctx->get_item();
	// and document
	auto d = doc->get_document();

	m_pagerec = std::shared_ptr<ThreadSafeVector<Math::Rectangle<float>>>
		(new ThreadSafeVector<Math::Rectangle<float>>(std::vector<Math::Rectangle<float>>()));

	// load in all pages
	auto page_count = get_page_count();
	for (size_t i = 0; i < page_count; i++) {
		m_pages.push_back(std::shared_ptr<PageWrapper>(new PageWrapper(ctx, fz_load_page(*c, *d, static_cast<int>(i))))); 
	}

	sort_page_positions();
}

MuPDFHandler::PDF::PDF(PDF&& t) noexcept {
	m_ctx = std::move(t.m_ctx);
	m_document = std::move(t.m_document);

	m_pages = std::move(t.m_pages);
	m_pagerec = std::move(t.m_pagerec);

	data = t.data;
	t.data = nullptr;

	size = t.size;
	t.size = 0;
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

std::shared_ptr<ThreadSafeVector<Math::Rectangle<float>>> MuPDFHandler::PDF::get_pagerec() {
	return m_pagerec;
}

ThreadSafePageWrapper MuPDFHandler::PDF::get_page(size_t page) {
	return m_pages.at(page)->get_item();
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
		opt.do_compress_images = 1;
		opt.do_compress = 1;
		opt.do_garbage = 1;

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

	SessionVariables::PDF_UNSAVED_CHANGES = false;

	if (*(SessionVariables::WINDOW_TITLE.end() - 1) == '*') {
		SessionVariables::WINDOW_TITLE.pop_back(); 
	}
}

void MuPDFHandler::PDF::add_page(PDF& pdf, int page_num) {
	auto c = m_ctx->get_context();
	auto d = m_document->get_document();

	fz_device* page_write = nullptr;
	pdf_obj* obj = nullptr; 
	pdf_obj* page = nullptr;
	fz_buffer* content = nullptr; 

	fz_try(*c) {
		auto rec = pdf.get_page_size(0);
		auto new_page_doc = pdf.get_document();

		page_write = pdf_page_write(*c, reinterpret_cast<pdf_document*>(*d), rec, &obj, &content); 

		fz_run_page(*c, reinterpret_cast<fz_page*>(*pdf.get_page(0)), page_write, fz_identity, nullptr); 

		page = pdf_add_page(*c, reinterpret_cast<pdf_document*>(*d), rec, 0, obj, content);
		
		pdf_insert_page(*c, reinterpret_cast<pdf_document*>(*d), INT_MAX, page);
	} 
	fz_always(*c) { 
		fz_close_device(*c, page_write);
		fz_drop_device(*c, page_write);
		pdf_drop_obj(*c, obj); 
		pdf_drop_obj(*c, page); 
		fz_drop_buffer(*c, content); 
	}
	fz_catch(*c) {
		fz_report_error(*c);
	} 

	m_pages.push_back(std::shared_ptr<PageWrapper>(new PageWrapper(m_ctx, fz_load_page(*c, *d, static_cast<int>(get_page_count() - 1))))); 

	// TODO in the future we need to do this dynamically since pages can be moved around
	sort_page_positions();
}

Math::Rectangle<float> MuPDFHandler::PDF::get_page_size(size_t page, float dpi) { 
	auto c = m_ctx->get_context();
	auto d = m_document->get_document();
	auto p = get_page(page);

	Math::Rectangle<float> rect;
	rect = fz_bound_page(*c, *p);
	return Math::Rectangle<float>(0, 0, rect.width * (dpi/ MUPDF_DEFAULT_DPI), rect.height * (dpi/ MUPDF_DEFAULT_DPI));
}

Math::Rectangle<float> MuPDFHandler::PDF::get_bounds() const {
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

	return Math::Rectangle<float>(left, up, right - left, down - up); 
} 


void MuPDFHandler::PDF::sort_page_positions() {
	auto dest = m_pagerec->get_write();
	dest->clear();
	double height = 0;
	for (size_t i = 0; i < get_page_count(); i++) {
		auto size = get_page_size(i);
		dest->push_back(Math::Rectangle<double>(0, height, size.width, size.height));
		height += get_page_size(i).height;
		height += AppVariables::PDF_SEPERATION_DISTANCE; 
	}
}