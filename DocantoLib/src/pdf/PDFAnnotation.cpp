#include "PDFAnnotation.h"

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>


struct AnnotationWrapper {
	pdf_annot* obj = nullptr;

	AnnotationWrapper(pdf_annot* a) : obj(a) {
	}

	AnnotationWrapper(const AnnotationWrapper&) = delete;
	AnnotationWrapper& operator=(const AnnotationWrapper& o) = delete;

	AnnotationWrapper(AnnotationWrapper&& o) noexcept : obj(std::exchange(o.obj, nullptr)) {}

	AnnotationWrapper& operator=(AnnotationWrapper&& o) noexcept {
		auto ctx = Docanto::GlobalPDFContext::get_instance().get();
		if (this != &o) {
			if (obj) pdf_drop_annot(*ctx, obj);
			obj = std::exchange(o.obj, nullptr);
		}
		return *this;
	}

	~AnnotationWrapper() {
		auto ctx = Docanto::GlobalPDFContext::get_instance().get();
		pdf_drop_annot(*ctx, obj);
	}
};

Docanto::PDFAnnotation::AnnotationType to_annot_type(enum pdf_annot_type t);

struct Docanto::PDFAnnotation::impl {
	std::vector<std::vector<std::pair<std::shared_ptr<AnnotationInfo>, AnnotationWrapper>>> all_annotations;

	impl() = default;
	~impl() = default;
};


std::shared_ptr<Docanto::PDFAnnotation::AnnotationInfo> do_general_annot(pdf_annot* a, std::shared_ptr<Docanto::PDFAnnotation::AnnotationInfo> info) {
	// in the case the info is empty we create a new one here
	if (info == nullptr) {
		info = std::make_shared<Docanto::PDFAnnotation::AnnotationInfo>();
	}

	auto ctx = Docanto::GlobalPDFContext::get_instance().get();
	// bounding box
	auto rec = pdf_bound_annot(*ctx, a);
	info->bounding_box = { rec.x0, rec.y0, rec.x1 - rec.x0, rec.y1 - rec.y0 };

	// annotation type
	info->type = to_annot_type(pdf_annot_type(*ctx, a));
	
	// color (if available)
	int n = 0;
	float c[4];
	pdf_annot_color(*ctx, a, &n, c);
	byte alpha = static_cast<byte>(pdf_annot_opacity(*ctx, a) * 255.0f);
	if (n == 1) {
		byte col = static_cast<byte>(c[0] * 255.0f);
		info->col = { col, col, col, alpha };
	}
	else if (n == 3) {
		info->col = {
			static_cast<byte>(c[0] * 255.0f),
			static_cast<byte>(c[1] * 255.0f),
			static_cast<byte>(c[2] * 255.0f),
			alpha
		};
	}
	else {
		Docanto::Logger::warn("Annotation Color format is not supported");
	}

	return info;

}

std::shared_ptr<Docanto::PDFAnnotation::AnnotationInfo> do_ink_annot(pdf_annot* a) {
	auto info = std::make_shared<Docanto::PDFAnnotation::InkAnnotationInfo>();
	auto ctx = Docanto::GlobalPDFContext::get_instance().get();

	// width
	info->stroke_width = pdf_annot_border_width(*ctx, a);

	// points
	info->points = std::make_shared< std::vector<Docanto::Geometry::Point<float>>>();
	auto list_count = pdf_annot_ink_list_count(*ctx, a);
	for (int i = 0; i < list_count; i++) {
		auto vertex_count = pdf_annot_ink_list_stroke_count(*ctx, a, i);
		for (int k = 0; k < vertex_count; k++) {
			auto point = pdf_annot_ink_list_stroke_vertex(*ctx, a, i, k);
			info->points->push_back({ point.x, point.y });
		}
	}


	return info;
}

void Docanto::PDFAnnotation::parse_annotation() {
	Timer time;
	size_t count = 0;

	auto ctx = Docanto::GlobalPDFContext::get_instance().get();
	auto page_amount = pdf_obj->get_page_count();

	for (size_t curr_page = 0; curr_page < page_amount; curr_page++) {
		// get current page
		auto fzpage = pdf_obj->get_page(curr_page).get();
		pimpl->all_annotations.push_back({});

		pdf_annot* annot = pdf_first_annot(*ctx, reinterpret_cast<pdf_page*>(*fzpage));

		while (annot != nullptr) {
			std::shared_ptr<Docanto::PDFAnnotation::AnnotationInfo> info;
			count++;

			switch (to_annot_type(pdf_annot_type(*ctx, annot))) {
			case AnnotationType::INK_ANNOTATION:
			{
				info = do_ink_annot(annot);
				break;
			}
			default:
				break;
			}
			info = do_general_annot(annot, info);
			info->page = curr_page;


			pimpl->all_annotations.back().push_back({ info, pdf_keep_annot(*ctx, annot)});

			annot = pdf_next_annot(*ctx, annot);
		} 
	}

	Logger::log("Parsed ", count, " annotations in ", time);
}

Docanto::PDFAnnotation::PDFAnnotation(std::shared_ptr<PDF> pdf_obj) : pdf_obj(pdf_obj), pimpl(std::make_unique<impl>()) {
	parse_annotation();
}

Docanto::PDFAnnotation::~PDFAnnotation() {
}

void Docanto::PDFAnnotation::add_annotation(size_t page, const std::vector<Geometry::Point<float>>& all_ponts, Color c, float width) {
	auto fzpage = pdf_obj->get_page(page).get();
	auto ctx = Docanto::GlobalPDFContext::get_instance().get();

	pdf_annot* annot = pdf_create_annot(*ctx, reinterpret_cast<pdf_page*>(*fzpage), PDF_ANNOT_INK);
	int count[1] = { static_cast<int>(all_ponts.size()) };
	// the reason we can just put in the all points vector is that Point
	pdf_set_annot_ink_list(*ctx, annot, 1, count, reinterpret_cast<const fz_point*>(all_ponts.data()));

	pdf_set_annot_border_width(*ctx, annot, width);
	float fzcolor[3] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f };
	pdf_set_annot_color(*ctx, annot, 3, fzcolor);

	// get the bounding box
	float xmin = all_ponts[0].x;
	float ymin = all_ponts[0].y;
	float xmax = all_ponts[0].x;
	float ymax = all_ponts[0].y;

	for (auto& p : all_ponts) {
		xmin = std::min(xmin, p.x);
		ymin = std::min(ymin, p.y);

		xmax = std::max(xmax, p.x);
		ymax = std::max(ymax, p.y);
	}
	// this doesnt work? i dont know how to set the bounding box in mupdf
	// but it seems like that when saving it also sets the bounding box
	//pdf_set_annot_rect(*ctx, annot, new_bound);

	auto info = std::make_shared<Docanto::PDFAnnotation::InkAnnotationInfo>();
	// bounding box
	info->bounding_box = { xmin, ymin, xmax - xmin, ymax - ymin };

	info->col = c;
	info->stroke_width = width;
	info->type = PDFAnnotation::AnnotationType::INK_ANNOTATION;
	
	pimpl->all_annotations[page].push_back({ info, annot });
}

std::vector<std::shared_ptr<Docanto::PDFAnnotation::AnnotationInfo>> Docanto::PDFAnnotation::get_annotation(size_t page, Geometry::Rectangle<float> rec) {
	std::vector<std::shared_ptr<Docanto::PDFAnnotation::AnnotationInfo>> all_annots;

	for (size_t i = 0; i < pimpl->all_annotations[page].size(); i++) {
		auto& annot = pimpl->all_annotations[page][i];

		if (annot.first->intersects(rec)) {
			all_annots.push_back(annot.first);
		}
	}

	return all_annots;
}



Docanto::PDFAnnotation::AnnotationType to_annot_type(enum pdf_annot_type t) {
	switch (t) {
	case PDF_ANNOT_TEXT:
		break;
	case PDF_ANNOT_LINK:
		break;
	case PDF_ANNOT_FREE_TEXT:
		break;
	case PDF_ANNOT_LINE:
		break;
	case PDF_ANNOT_SQUARE:
		break;
	case PDF_ANNOT_CIRCLE:
		break;
	case PDF_ANNOT_POLYGON:
		break;
	case PDF_ANNOT_POLY_LINE:
		break;
	case PDF_ANNOT_HIGHLIGHT:
		break;
	case PDF_ANNOT_UNDERLINE:
		break;
	case PDF_ANNOT_SQUIGGLY:
		break;
	case PDF_ANNOT_STRIKE_OUT:
		break;
	case PDF_ANNOT_REDACT:
		break;
	case PDF_ANNOT_STAMP:
		break;
	case PDF_ANNOT_CARET:
		break;
	case PDF_ANNOT_INK:
		return Docanto::PDFAnnotation::AnnotationType::INK_ANNOTATION;
	case PDF_ANNOT_POPUP:
		break;
	case PDF_ANNOT_FILE_ATTACHMENT:
		break;
	case PDF_ANNOT_SOUND:
		break;
	case PDF_ANNOT_MOVIE:
		break;
	case PDF_ANNOT_RICH_MEDIA:
		break;
	case PDF_ANNOT_WIDGET:
		break;
	case PDF_ANNOT_SCREEN:
		break;
	case PDF_ANNOT_PRINTER_MARK:
		break;
	case PDF_ANNOT_TRAP_NET:
		break;
	case PDF_ANNOT_WATERMARK:
		break;
	case PDF_ANNOT_3D:
		break;
	case PDF_ANNOT_PROJECTION:
		break;
	case PDF_ANNOT_UNKNOWN:
		break;
	default:
		break;
	}

	return Docanto::PDFAnnotation::AnnotationType::UNKNOWN;
}

bool Docanto::PDFAnnotation::AnnotationInfo::intersects(Geometry::Rectangle<float> other) {
	return bounding_box.intersects(other);
}

bool Docanto::PDFAnnotation::InkAnnotationInfo::intersects(Geometry::Rectangle<float> other) {
	if (!bounding_box.intersects(other)) {
		return false;
	}

	for (size_t i = 0; i < points->size(); i++) {
		if (other.intersects(points->at(i))) {
			return true;
		}
	}

	return false;
}
