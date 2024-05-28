#include "include.h"

ContextWrapper::ContextWrapper(fz_context* c) {
	m_ctx = c;
}

ThreadSafeContextWrapper ContextWrapper::get_context() {
	return ThreadSafeContextWrapper(m_mutex, &m_ctx);
}

ContextWrapper::~ContextWrapper() {
	std::unique_lock<std::mutex>(m_mutex);
	fz_drop_context(m_ctx);
}


DocumentWrapper::DocumentWrapper(std::shared_ptr<ContextWrapper> a, fz_document* d) {
	m_context = a;
	m_doc = d;
}

ThreadSafeDocumentWrapper DocumentWrapper::get_document() {
	return ThreadSafeDocumentWrapper(m_mutex, &m_doc);
}

DocumentWrapper::~DocumentWrapper() {
	auto ctx = m_context->get_context();
	auto doc = get_document();
	fz_drop_document(*ctx, *doc);
}

PageWrapper::PageWrapper(std::shared_ptr<ContextWrapper> a, fz_page* p) {
	m_context = a;
	m_pag = p;
}

ThreadSafePageWrapper PageWrapper::get_page() {
	return ThreadSafePageWrapper(m_mutex, &m_pag);
}

PageWrapper::~PageWrapper() {
	auto ctx = m_context->get_context();
	auto pag = get_page();
	fz_drop_page(*ctx, *pag);
}

DisplayListWrapper::DisplayListWrapper(std::shared_ptr<ContextWrapper> a, fz_display_list* l) {
	m_context = a;
	m_lis = l;
}

ThreadSafeDisplayListWrapper DisplayListWrapper::get_displaylist() {
	return ThreadSafeDisplayListWrapper(m_mutex, &m_lis);
}

DisplayListWrapper::~DisplayListWrapper() {
	auto ctx = m_context->get_context();
	auto lis = get_displaylist();
	fz_drop_display_list(*ctx, *lis);
}
