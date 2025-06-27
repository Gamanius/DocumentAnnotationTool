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
}

Docanto::PDF::~PDF() {
	auto doc = this->get();
	auto ctx = GlobalPDFContext::get_instance().get();

	fz_drop_document(*ctx, *doc);
}

size_t Docanto::PDF::get_page_count() {
	auto ctx = GlobalPDFContext::get_instance().get();
	auto doc = this->get();

	return fz_count_pages(*ctx, *doc);
}