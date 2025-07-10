#ifndef _PDFRENDERER_H_
#define _PDFRENDERER_H_

#include "../general/Common.h"
#include "../general/Image.h"
#include "PDF.h"


namespace Docanto {
	class IPDFRenderImageProcessor {
	public:
		virtual ~IPDFRenderImageProcessor() = default;

		virtual void processImage(size_t id, const Image& img) = 0;
	};

	class PDFRenderer {
		struct PDFRenderInfo {
			size_t id = 0;
			Geometry::Point<float> pos;
			float dpi = 0;
		};

		std::shared_ptr<PDF> pdf_obj;

		std::shared_ptr<IPDFRenderImageProcessor> m_processor;

		float m_standard_dpi = 96;

		void create_preview(float dpi = MUPDF_DEFAULT_DPI);
	public:
		PDFRenderer(std::shared_ptr<PDF> pdf_obj, std::shared_ptr<IPDFRenderImageProcessor> processor);
		~PDFRenderer();

		Image get_image(size_t page, float dpi = MUPDF_DEFAULT_DPI);

		const std::vector<PDFRenderInfo>& get_preview();

		void update();
	private:
		struct impl;

		std::unique_ptr<impl> pimpl;
	};
}


#endif // !_PDFRENDERER_H_
