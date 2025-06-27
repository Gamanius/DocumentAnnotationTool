#include "PDFRenderer.h"

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

Docanto::PDFRenderer::PDFRenderer(std::shared_ptr<PDF> pdf_obj) {
	this->pdf_obj = pdf_obj;
}

Docanto::Image Docanto::PDFRenderer::get_image(size_t page, float dpi) {
	fz_matrix ctm;
	auto size = pdf_obj->get_page_dimension(page) * dpi / MUPDF_DEFAULT_DPI;
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;

	auto ctx = GlobalPDFContext::get_instance().get();
	auto doc = pdf_obj->get();
	auto pag = pdf_obj->get_page(page).get();

	Image obj;

	fz_try(*ctx) {
		pixmap = fz_new_pixmap(*ctx, fz_device_rgb(*ctx), size.width, size.height, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, *pag, drawdevice, ctm, nullptr);
		fz_close_device(*ctx, drawdevice);

		obj.data = std::unique_ptr<byte>(pixmap->samples); // , size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
		obj.dims = size ;
		obj.size = pixmap->h * pixmap->stride;
		obj.stride = pixmap->stride;
		obj.components = pixmap->n;

		pixmap->samples = nullptr;
	} fz_always(*ctx) {
		fz_drop_device(*ctx, drawdevice);
		fz_drop_pixmap(*ctx, pixmap);
	} fz_catch(*ctx) {
		return Docanto::Image();
	}

	return std::move(obj);
}
