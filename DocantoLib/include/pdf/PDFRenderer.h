#ifndef _PDFRENDERER_H_
#define _PDFRENDERER_H_

#include "../general/Common.h"
#include "../general/Image.h"
#include "PDF.h"

constexpr float MUPDF_DEFAULT_DPI = 72.0f;

namespace Docanto {
	class PDFRenderer {
		std::shared_ptr<PDF> pdf_obj;
	public:
		PDFRenderer(std::shared_ptr<PDF> pdf_obj);
		~PDFRenderer();

		Image get_image(size_t page, float dpi = MUPDF_DEFAULT_DPI);

		void update();
	private:
		struct impl;

		std::unique_ptr<impl> pimpl;
	};
}


#endif // !_PDFRENDERER_H_
