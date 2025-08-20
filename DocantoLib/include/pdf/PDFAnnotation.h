#ifndef _DOCANTO_PDFANNOTATION_H_
#define _DOCANTO_PDFANNOTATION_H_

#include "general/Common.h"
#include "general/BasicRender.h"
#include "general/Timer.h"
#include "PDF.h"

namespace Docanto {


	class PDFAnnotation {
		std::shared_ptr<PDF> pdf_obj;

		void parse_annotation();
	public:
		enum class AnnotationType {
			UNKNOWN,
			INK_ANNOTATION
		};

		struct AnnotationInfo {
			size_t page = 0;
			AnnotationType type = AnnotationType::UNKNOWN;
			/// <summary>
			/// given in local doc space
			/// </summary>
			Geometry::Rectangle<float> bounding_box;
			Color col;

			virtual bool intersects(Geometry::Rectangle<float> other);
		};

		struct InkAnnotationInfo : public AnnotationInfo {
			std::shared_ptr<std::vector<Geometry::Point<float>>> points;
			float stroke_width = 1.0f;

			bool intersects(Geometry::Rectangle<float> other) override;
		};

		PDFAnnotation(std::shared_ptr<PDF> pdf_obj);
		~PDFAnnotation();

		PDFAnnotation(const PDFAnnotation&) = delete;
		PDFAnnotation& operator=(const PDFAnnotation&) = delete;

		void add_annotation(size_t page, const std::vector<Geometry::Point<float>>& all_ponts, Color c = {}, float width = 1);
		std::vector<std::shared_ptr<AnnotationInfo>> get_annotation(size_t page, Geometry::Rectangle<float> rec);

	private:
		struct impl;

		std::unique_ptr<impl> pimpl;
	};
}

#endif
