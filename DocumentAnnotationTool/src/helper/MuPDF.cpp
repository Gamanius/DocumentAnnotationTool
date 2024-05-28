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

	m_context = std::shared_ptr<ContextWrapper>(new ContextWrapper(fz_new_context(nullptr, &m_locks, FZ_STORE_UNLIMITED)));
	
	auto c = m_context->get_item();

	fz_set_error_callback(*c, error_callback, nullptr);
	fz_register_document_handlers(*c);
}

std::optional<MuPDFHandler::PDF> MuPDFHandler::load_pdf(const std::wstring& path) {
	auto ctx = get_context();

	// load the pdf 
	auto file = FileHandler::open_file(path);
	ASSERT_RETURN_NULLOPT(file.has_value(), L"Could not open file " + path);

	auto stream = fz_open_memory(*ctx, file.value().data, file.value().size);
	auto doc = fz_open_document_with_stream(*ctx, ".pdf", stream);

	auto d = std::shared_ptr<DocumentWrapper>(new DocumentWrapper(m_context, doc));
	fz_drop_stream(*ctx, stream);

	ASSERT_RETURN_NULLOPT(doc != nullptr, L"Could not open document " + path); 

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

MuPDFHandler::PDF::PDF(std::shared_ptr<ContextWrapper> ctx, std::shared_ptr<DocumentWrapper> doc) {
	m_ctx = ctx;
	m_document = doc; 

	// get context
	auto c = ctx->get_item();
	// and document
	auto d = doc->get_document();

	for (size_t i = 0; i < get_page_count(); i++) { 
		auto page = fz_load_page(*c, *d, i);
		m_pages.push_back(std::shared_ptr<PageWrapper>(new PageWrapper(ctx, page))); 
	}
}

MuPDFHandler::PDF::PDF(PDF&& t) noexcept {
	m_document = std::move(t.m_document);

	m_ctx = std::move(t.m_ctx);

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
	// pages are going to be deleted by the wrapper
	// so there is nothing ot be deleted
}

ThreadSafePageWrapper MuPDFHandler::PDF::get_page(size_t page) const {
	return m_pages.at(page)->get_page();
}

ThreadSafeContextWrapper MuPDFHandler::PDF::get_context() const {
	return m_ctx->get_item();
}

std::shared_ptr<ContextWrapper> MuPDFHandler::PDF::get_context_wrapper() const {
	return m_ctx;
}

size_t MuPDFHandler::PDF::get_page_count() const { 
	auto c = m_ctx->get_context();
	auto d = m_document->get_document();
	return fz_count_pages(*c, *d);
}

Renderer::Rectangle<float> MuPDFHandler::PDF::get_page_size(size_t page, float dpi) const {
	auto c = m_ctx->get_context();
	auto p = m_pages.at(page)->get_page();

	Renderer::Rectangle<float> rect;
	rect = fz_bound_page(*c, *p);
	return Renderer::Rectangle<float>(0, 0, rect.width * (dpi/ MUPDF_DEFAULT_DPI), rect.height * (dpi/ MUPDF_DEFAULT_DPI));
}
