#include "PDFContext.h"
#include "mupdf/pdf.h"
#include "../general/File.h"

std::mutex m_mutex[FZ_LOCK_MAX];


void lock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].lock();
}

void unlock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].unlock();
}

fz_locks_context lock_content = {
	m_mutex,
	lock_mutex,
	unlock_mutex
};

void error_callback(void* user, const char* message) {
	Docanto::Logger::error("MuPDF: \"", message, "\"");
}

void warning_callback(void* user, const char* message) {
	Docanto::Logger::warn("MuPDF: \"", message, "\"");
}

Docanto::PDFContext::PDFContext() : ThreadSafeWrapper(fz_new_context(nullptr, &lock_content, FZ_STORE_UNLIMITED)) {
	auto c = get();
	
	fz_set_error_callback(*c, error_callback, nullptr);
	fz_set_warning_callback(*c, warning_callback, nullptr);
	fz_register_document_handlers(*c);
}

Docanto::PDFContext::~PDFContext() {
	auto ctx = get();
	fz_drop_context(*ctx);


}


Docanto::PDFContext& Docanto::PDFContext::get_instance() {
	static PDFContext instance;
	return instance;
}

