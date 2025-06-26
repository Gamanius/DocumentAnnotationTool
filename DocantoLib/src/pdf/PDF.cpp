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

	auto ctx = PDFContext::get_instance().get();

	auto stream = fz_open_memory(*ctx, this->data, file.value().size);
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