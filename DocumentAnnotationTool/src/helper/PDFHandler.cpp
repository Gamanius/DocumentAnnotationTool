#include "include.h"

void PDFHandler::lock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].lock();
}

void PDFHandler::unlock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].unlock();
}

void PDFHandler::error_callback(void* user, const char* message) {
	ASSERT(false, "Error in PDFHandler: " + std::string(message));
}

PDFHandler::PDFHandler() { 
	m_locks.user = m_mutex; 
	m_locks.lock = lock_mutex; 
	m_locks.unlock = unlock_mutex; 

	m_ctx = fz_new_context(nullptr, &m_locks, FZ_STORE_UNLIMITED);
	fz_set_error_callback(m_ctx, error_callback, nullptr); 
	fz_register_document_handlers(m_ctx);
}

std::optional<PDFHandler::PDF> PDFHandler::load_pdf(const std::wstring& path) {
	// load the pdf 
	auto file = FileHandler::open_file(path);
	ASSERT_RETURN_NULLOPT(file.has_value(), L"Could not open file " + path);

	auto stream = fz_open_memory(m_ctx, file.value().data, file.value().size);
	auto doc = fz_open_document_with_stream(m_ctx, ".pdf", stream);
	fz_drop_stream(m_ctx, stream);

	ASSERT_RETURN_NULLOPT(doc != nullptr, L"Could not open document " + path); 

	auto pdf = PDF(m_ctx);
	pdf.data = file.value().data;
	file.value().data = nullptr;
	pdf.size = file.value().size;
	pdf.m_doc = doc;

	return std::move(pdf); 
}

PDFHandler::~PDFHandler() {
	fz_drop_context(m_ctx);
}

// PDF CODE //

PDFHandler::PDF::PDF(fz_context* ctx) {
	m_ctx = ctx;
}

PDFHandler::PDF::PDF(PDF&& t) noexcept {
	m_doc = t.m_doc;
	t.m_doc = nullptr;

	m_ctx = t.m_ctx;
	t.m_ctx = nullptr;

	data = t.data;
	t.data = nullptr;
}


PDFHandler::PDF& PDFHandler::PDF::operator=(PDF&& t) noexcept {
	if (this != &t) {
		m_doc = t.m_doc;
		t.m_doc = nullptr;

		m_ctx = t.m_ctx;
		t.m_ctx = nullptr;

		data = t.data;
		t.data = nullptr;
	}
	return *this;
}

PDFHandler::PDF::~PDF() {
	fz_drop_document(m_ctx, m_doc);
}

size_t PDFHandler::PDF::get_page_count() const {
	return fz_count_pages(m_ctx, m_doc);
}
