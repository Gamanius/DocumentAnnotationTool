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

PageWrapper::PageWrapper(std::shared_ptr<ContextWrapper> a, fz_page* p) {
	m_context = a;
	page = p;
}

PageWrapper::PageWrapper(PageWrapper&& t) noexcept {
	page = t.page;
	t.page = nullptr;

	m_context = std::move(t.m_context);

}

PageWrapper& PageWrapper::operator=(PageWrapper&& t) noexcept {
	this->~PageWrapper(); 
	new(this) PageWrapper(std::move(t));
	return *this;
}

PageWrapper::operator fz_page* () const {
	return page;
}

fz_page* PageWrapper::get_page() const {
	return page;
}

PageWrapper::~PageWrapper() {
	if (page == nullptr)
		return;
	auto ctx = m_context->get_context();
	fz_drop_page(*ctx, page);
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
