#include "include.h"

ContextWrapper::ContextWrapper(fz_context* c) : ThreadSafeWrapper<fz_context*>(std::move(c)) {}

ThreadSafeContextWrapper ContextWrapper::get_context() {
	return this->get_item();
}


ContextWrapper::~ContextWrapper() {
	auto c = this->get_item(); 
	fz_drop_context(*c);
}


DocumentWrapper::DocumentWrapper(std::shared_ptr<ContextWrapper> a, fz_document* d) : ThreadSafeWrapper<fz_document*>(std::move(d)) {
	m_context = a;
}

ThreadSafeDocumentWrapper DocumentWrapper::get_document() {
	return this->get_item();
}

DocumentWrapper::~DocumentWrapper() {
	auto ctx = m_context->get_context();
	auto doc = get_document();
	fz_drop_document(*ctx, *doc);
}

PageWrapper::PageWrapper(std::shared_ptr<ContextWrapper> a, fz_page* p) : ThreadSafeWrapper<fz_page*>(std::move(p)) {
	m_context = a;
}

ThreadSafePageWrapper PageWrapper::get_page() {
	return this->get_item();
}

PageWrapper::~PageWrapper() {
	auto ctx = m_context->get_context();
	auto pag = get_page();
	fz_drop_page(*ctx, *pag);
}

DisplayListWrapper::DisplayListWrapper(std::shared_ptr<ContextWrapper> a, fz_display_list* l) : ThreadSafeWrapper<fz_display_list*>(std::move(l)) {
	m_context = a;
}

ThreadSafeDisplayListWrapper DisplayListWrapper::get_displaylist() {
	return this->get_item();
}

DisplayListWrapper::~DisplayListWrapper() {
	auto ctx = m_context->get_context();
	auto lis = get_displaylist();
	fz_drop_display_list(*ctx, *lis);
}
