#include "pdf.h"
#include "mupdf/pdf.h"

Docanto::PDF::PDF(const std::filesystem::path& p) {
	auto file = File::load(p);

	if (!file.has_value()) {
		return;
	}

	// implicitly move the gained values
	this->data = std::move(file.value().data);
	this->size = std::move(file.value().size);
	this->path = std::move(file.value().path);

	auto ctx = GlobalPDFContext::get_instance().get();
	auto stream = fz_open_memory(*ctx, this->data.get(), this->size);
	auto doc = fz_open_document_with_stream(*ctx, ".pdf", stream);

	this->set(std::move(doc));

	fz_drop_stream(*ctx, stream);

	if (doc == nullptr) {
		Logger::error(L"Could not open document ", p);
	}

	// at the end we have to load in the pages
	auto page_count = get_page_count();

	for (size_t i = 0; i < page_count; i++) {
		m_pages.emplace_back(std::make_unique<PageWrapper>(fz_load_page(*ctx, doc, static_cast<int>(i))));
	}

	Logger::success("Loaded in PDF at path ", path);
}

Docanto::PDF::~PDF() {
	auto doc = this->get();
	auto ctx = GlobalPDFContext::get_instance().get();

	for (size_t i = 0; i < m_pages.size(); i++) {
		auto pag = m_pages.at(i).get();
		fz_drop_page(*ctx, *(pag->get()));
	}

	fz_drop_document(*ctx, *doc);
}

size_t Docanto::PDF::get_page_count() {
	auto ctx = GlobalPDFContext::get_instance().get();
	auto doc = this->get();

	return fz_count_pages(*ctx, *doc);
}

Docanto::PageWrapper& Docanto::PDF::get_page(size_t page) {
	return *(m_pages.at(page).get());
}

Docanto::Geometry::Dimension<float> Docanto::PDF::get_page_dimension(size_t page) {
	auto ctx = GlobalPDFContext::get_instance().get();

	auto s = fz_bound_page(*ctx, *(get_page(page).get()));
	
	return { s.x1 - s.x0, s.y1 - s.y0 };
}