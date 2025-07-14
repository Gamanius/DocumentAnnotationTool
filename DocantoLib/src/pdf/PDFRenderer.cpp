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
	ThreadSafeVector<Geometry::Point<float>> m_page_pos;

	std::atomic_size_t m_last_id = 0;
	ThreadSafeVector<PDFRenderInfo> m_previewbitmaps;
	ThreadSafeVector<PDFRenderInfo> m_highDefBitmaps;


	Geometry::Rectangle<float> m_current_viewport;
	float m_current_dpi;

	impl() = default;
	~impl() = default;
};

Docanto::PDFRenderer::PDFRenderer(std::shared_ptr<PDF> pdf_obj, std::shared_ptr<IPDFRenderImageProcessor> processor) : pimpl(std::make_unique<impl>()) {
	this->pdf_obj = pdf_obj;
	this->m_processor = processor;

	position_pdfs();
	create_preview();
	update();
}

Docanto::PDFRenderer::~PDFRenderer() = default;

Docanto::Image get_image_from_list(DisplayListWrapper* wrap, Docanto::Geometry::Rectangle<float> scissor, float dpi) {
	// now we can render it
	fz_matrix ctm;
	auto scale = dpi / MUPDF_DEFAULT_DPI;
	ctm = fz_scale(scale, scale);
	auto pixmap_scale = fz_scale(dpi/96, dpi/96);
	auto transform = fz_translate(-scissor.x, -scissor.y);
	// this is to calculate the size of the pixmap using the source
	auto pixmap_size = fz_irect_from_rect(fz_transform_rect(fz_make_rect(scissor.x, scissor.y, scissor.width, scissor.height), pixmap_scale));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;

	auto ctx = Docanto::GlobalPDFContext::get_instance().get();

	Docanto::Image obj;
	fz_try(*ctx) {
		// ___---___ Rendering part ___---___
		// create new pixmap
		pixmap = fz_new_pixmap_with_bbox(*ctx, fz_device_rgb(*ctx), pixmap_size, nullptr, 1);
		// create draw device
		drawdevice = fz_new_draw_device(*ctx, fz_concat(transform, ctm), pixmap);
		// render to draw device
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		fz_run_display_list(*ctx, *(wrap->get().get()), drawdevice, fz_identity, fz_make_rect(scissor.x, scissor.y, scissor.right(), scissor.bottom()), nullptr);

		fz_close_device(*ctx, drawdevice);
		// ___---___ Bitmap creatin and display part ___---___
		// create the bitmap
		obj.data = std::unique_ptr<byte>(pixmap->samples); // , size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
		obj.dims = { (size_t)(pixmap_size.x1 - pixmap_size.x0), (size_t)(pixmap_size.y1 - pixmap_size.y0) };
		obj.size = pixmap->h * pixmap->stride;
		obj.stride = pixmap->stride;
		obj.components = pixmap->n;
		obj.dpi = dpi;

		pixmap->samples = nullptr;

	} fz_always(*ctx) {
		// drop all devices
		fz_drop_device(*ctx, drawdevice);
		fz_drop_pixmap(*ctx, pixmap);
	} fz_catch(*ctx) {
		Docanto::Logger::log("MUPdf Error: ", fz_convert_error(*ctx, &((*ctx)->error.errcode)));
	}
	return obj;
}


std::vector<Docanto::Geometry::Rectangle<float>> Docanto::PDFRenderer::chunk(size_t page) {
	auto dims = pdf_obj->get_page_dimension(page, m_standard_dpi);
	auto  pos = pimpl->m_page_pos.get_read()->at(page);

	size_t scale = std::floor(pimpl->m_current_dpi / m_standard_dpi);
	size_t amount_cells = 3 * scale + 1;
	Geometry::Dimension<float> cell_dim = { dims.width / amount_cells, dims.height / amount_cells };
	
	// transform the viewport to docspace
	auto doc_space_screen = Docanto::Geometry::Rectangle<float>(pimpl->m_current_viewport.upperleft() - pos, pimpl->m_current_viewport.lowerright() - pos);

	Geometry::Point<size_t> topleft = {
		(size_t)std::clamp(std::floor(doc_space_screen.upperleft().x /  dims.width * amount_cells), 0.0f, (float)amount_cells),
		(size_t)std::clamp(std::floor(doc_space_screen.upperleft().y / dims.height * amount_cells), 0.0f, (float)amount_cells),
	};

	Geometry::Point<size_t> bottomright = {
		(size_t)std::clamp(std::ceil(doc_space_screen.lowerright().x / dims.width * amount_cells), 0.0f, (float)amount_cells),
		(size_t)std::clamp(std::ceil(doc_space_screen.lowerright().y / dims.height * amount_cells), 0.0f, (float)amount_cells),
	};

	auto chunks = std::vector<Docanto::Geometry::Rectangle<float>>();
	auto dx = bottomright.x - topleft.x;
	auto dy = bottomright.y - topleft.y;
	chunks.reserve(dx * dy);

	for (size_t x = topleft.x; x < bottomright.x; ++x) {
		for (size_t y = topleft.y; y < bottomright.y; ++y) {
			chunks.push_back({
				x * cell_dim.width,
				y * cell_dim.height,
				cell_dim.width,
				cell_dim.height
				});
		}
	}

	return chunks;
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


void Docanto::PDFRenderer::position_pdfs() {
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto positions = pimpl->m_page_pos.get_write();

	float y = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i, m_standard_dpi);
		positions->push_back({0, y});
		y += dims.height + 10;
	}
}

void Docanto::PDFRenderer::create_preview(float dpi) {
	Docanto::Logger::log("Creating preview");
	Timer t;
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto   positions       = pimpl->m_page_pos.get_read();

	float y = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i, m_standard_dpi);
		auto id = pimpl->m_last_id++;
		m_processor->processImage(id, get_image(i, dpi));

		auto prevs = pimpl->m_previewbitmaps.get_write();
		prevs->push_back({ id, {positions->at(i), dims}, dpi });

		y += dims.height + 10;
	}
	Docanto::Logger::log("Preview generation and processing took ", t);
}

const std::vector<Docanto::PDFRenderer::PDFRenderInfo>& Docanto::PDFRenderer::get_preview() {
	auto d =  pimpl->m_previewbitmaps.get_read();
	return *d.get();
}

const std::vector<Docanto::PDFRenderer::PDFRenderInfo>& Docanto::PDFRenderer::draw() {
	auto d =  pimpl->m_highDefBitmaps.get_read();
	return *d.get();
}


void Docanto::PDFRenderer::request(Geometry::Rectangle<float> view, float dpi) {
	pimpl->m_current_viewport = view;
	pimpl->m_current_dpi = dpi;
}

void Docanto::PDFRenderer::render() {
	{
		auto vec = pimpl->m_highDefBitmaps.get_write();
		for (auto &item : *vec.get()) {
			m_processor->deleteImage(item.id);
		}
		vec->clear();
	}

	size_t amount_of_pages = pdf_obj->get_page_count();
	auto   positions = pimpl->m_page_pos.get_read();

	size_t amount = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i, m_standard_dpi);

		auto pdf_rec = Geometry::Rectangle<float>(positions->at(i), dims);

		if (!pdf_rec.intersects(pimpl->m_current_viewport)) {
			continue;
		}

		auto cont = pimpl->m_page_content.get_read()->at(i).get();
		auto img = get_image_from_list(cont, {{0,0},dims}, pimpl->m_current_dpi);
		auto id = pimpl->m_last_id++;
		m_processor->processImage(id, img);

		auto prevs = pimpl->m_highDefBitmaps.get_write();
		prevs->push_back({ id, pdf_rec, pimpl->m_current_dpi });
	}
}

void Docanto::PDFRenderer::debug_draw(std::shared_ptr<BasicRender> render) {
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto   positions = pimpl->m_page_pos.get_read();

	size_t amount = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i, m_standard_dpi);

		auto pdf_rec = Geometry::Rectangle<float>(positions->at(i), dims);

		if (pdf_rec.intersects(pimpl->m_current_viewport)) {
			render->draw_rect(pdf_rec, { 255 });
			amount++;
			auto recs = chunk(i);

			for (auto& r : recs) {
				r = { r.upperleft() + positions->at(i), Geometry::Dimension<float>(r.dims())};
				render->draw_rect(r, { 0, 255 });
			}
		}
	}

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