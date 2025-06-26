#include "PDFContext.h"
#include "mupdf/pdf.h"

struct Docanto::PDFContext::Impl {
	fz_context* m_ctx;
};

Docanto::PDFContext::PDFContext() {
	auto context = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);

	m_impl = std::unique_ptr<Impl>(new Impl());

	m_impl->m_ctx = context;
}

Docanto::PDFContext::~PDFContext() {
	fz_drop_context(m_impl->m_ctx);
}

Docanto::PDFContext& Docanto::PDFContext::get_instance() {
	static PDFContext instance;
	return instance;
}