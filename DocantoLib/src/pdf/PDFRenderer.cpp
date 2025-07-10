#include "PDFRenderer.h"

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

#include "../../include/general/Timer.h"
#include "../../include/general/ReadWriteMutex.h"

typedef struct {
	unsigned int cmd : 5;
	unsigned int size : 9;
	unsigned int rect : 1;
	unsigned int path : 1;
	unsigned int cs : 3;
	unsigned int color : 1;
	unsigned int alpha : 2;
	unsigned int ctm : 3;
	unsigned int stroke : 1;
	unsigned int flags : 6;
} fz_display_node;

struct fz_display_list {
	fz_storable storable;
	fz_display_node* list;
	fz_rect mediabox;
	size_t max;
	size_t len;
};

struct DisplayListWrapper : public Docanto::ThreadSafeWrapper<fz_display_list*> {
	using Docanto::ThreadSafeWrapper<fz_display_list*>::ThreadSafeWrapper;

	~DisplayListWrapper() {
		auto d = get().get();

		if (d != nullptr) {
			auto ctx = Docanto::GlobalPDFContext::get_instance().get();
			fz_drop_display_list(*ctx, *d);
		}
	}
};

struct Docanto::PDFRenderer::impl {
	ThreadSafeVector<std::unique_ptr<DisplayListWrapper>> m_page_content;
	ThreadSafeVector<std::unique_ptr<DisplayListWrapper>> m_page_widgets;
	ThreadSafeVector<std::unique_ptr<DisplayListWrapper>> m_page_annotat;

	std::atomic_size_t m_last_id = 0;
	ThreadSafeVector<PDFRenderInfo> m_previewbitmaps;


	impl() = default;
	~impl() = default;
};

Docanto::PDFRenderer::PDFRenderer(std::shared_ptr<PDF> pdf_obj, std::shared_ptr<IPDFRenderImageProcessor> processor) : pimpl(std::make_unique<impl>()) {
	this->pdf_obj = pdf_obj;
	this->m_processor = processor;
	create_preview();
}

Docanto::PDFRenderer::~PDFRenderer() = default;

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
		obj.dpi = dpi;

		pixmap->samples = nullptr;
	} fz_always(*ctx) {
		fz_drop_device(*ctx, drawdevice);
		fz_drop_pixmap(*ctx, pixmap);
	} fz_catch(*ctx) {
		return Docanto::Image();
	}

	return obj;
}


void Docanto::PDFRenderer::create_preview(float dpi) {
	Docanto::Logger::log("Creating preview");
	Timer t;
	size_t amount_of_pages = pdf_obj->get_page_count();

	float y = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i, m_standard_dpi);
		auto id = pimpl->m_last_id++;
		m_processor->processImage(id, get_image(i, dpi));

		auto prevs = pimpl->m_previewbitmaps.get_write();
		prevs->push_back({ id, {0, y}, dpi });

		y += dims.height + 10;
	}
	Docanto::Logger::log("Preview generation and processing took ", t);
}

const std::vector<Docanto::PDFRenderer::PDFRenderInfo>& Docanto::PDFRenderer::get_preview() {
	auto d =  pimpl->m_previewbitmaps.get_read();
	return *d.get();
}

void Docanto::PDFRenderer::update() {
	// clear the list and copy all again
	Logger::log(L"Start creating Display List");
	Docanto::Timer time;
	// for each rec create a display list
	fz_display_list* list_annot = nullptr;
	fz_display_list* list_widget = nullptr;
	fz_display_list* list_content = nullptr;
	fz_device* dev_annot = nullptr;
	fz_device* dev_widget = nullptr;
	fz_device* dev_content = nullptr;

	auto ctx = GlobalPDFContext::get_instance().get();

	size_t amount_of_pages = pdf_obj->get_page_count();
	//m_display_list_amount_processed_total = amount_of_pages;

	for (size_t i = 0; i < amount_of_pages; i++) {
		// get the page that will be rendered
		auto doc = pdf_obj->get();
		// we have to do a fz call since we want to use the ctx.
		// we can't use any other calls (like m_pdf->get_page()) since we would need to create a ctx
		// wrapper which would be overkill for this scenario.
		auto p = fz_load_page(*ctx, *doc, static_cast<int>(i));
		fz_try(*ctx) {
			// create a display list with all the draw calls and so on
			list_annot = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));
			list_widget = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));
			list_content = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));

			dev_annot = fz_new_list_device(*ctx, list_annot);
			dev_widget = fz_new_list_device(*ctx, list_widget);
			dev_content = fz_new_list_device(*ctx, list_content);

			// run all three devices
			Timer time2;
			fz_run_page_annots(*ctx, p, dev_annot, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Annots Rendered in ", time2);

			time2 = Timer();
			fz_run_page_widgets(*ctx, p, dev_widget, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Widgets Rendered in ", time2);

			time2 = Timer();
			fz_run_page_contents(*ctx, p, dev_content, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Content Rendered in ", time2);

			// get the list and add the lists
			auto content = pimpl->m_page_content.get_write();
			content->emplace_back(std::make_unique<DisplayListWrapper>(std::move(list_content)));

			auto widgets = pimpl->m_page_widgets.get_write();
			widgets->emplace_back(std::make_unique<DisplayListWrapper>(std::move(list_widget)));

			auto annotat = pimpl->m_page_annotat.get_write();
			annotat->emplace_back(std::make_unique<DisplayListWrapper>(std::move(list_annot)));


		} fz_always(*ctx) {
			// flush the device
			fz_close_device(*ctx, dev_annot);
			fz_close_device(*ctx, dev_widget);
			fz_close_device(*ctx, dev_content);
			fz_drop_device(*ctx, dev_annot);
			fz_drop_device(*ctx, dev_widget);
			fz_drop_device(*ctx, dev_content);
		} fz_catch(*ctx) {
			Docanto::Logger::error("Could not preprocess the PDF page");
		}
		// always drop page at the end
		fz_drop_page(*ctx, p);
	}

	Logger::log(L"Finished Displaylist in ", time);
}