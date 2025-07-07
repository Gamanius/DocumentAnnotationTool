#ifndef _DOCANTOWIN_PDFHANDLER_H_
#define _DOCANTOWIN_PDFHANDLER_H_

#include "../include.h"

#include "win/Direct2D.h"

namespace DocantoWin {
	class PDFHandler {
		std::shared_ptr<Docanto::PDF> m_pdf;
		std::shared_ptr<Docanto::PDFRenderer> m_pdfrender;

		std::shared_ptr<Direct2DRender> m_render;

		std::vector<std::unique_ptr<Direct2DRender::BitmapObject>> m_bitmaps;
	public:
		PDFHandler(const std::filesystem::path& p, std::shared_ptr<Direct2DRender> render);

		void render();
		void draw();

		std::shared_ptr<Docanto::PDF> get_pdf() const;
	};
}

#endif // !_DOCANTOWIN_PDFHANDLER_H_
