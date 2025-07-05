#ifndef _DOCANTOWIN_MAINWINDOWHANDLER_H_
#define _DOCANTOWIN_MAINWINDOWHANDLER_H_

#include "Window.h"
#include "Direct2D.h"

#include "ui/Caption.h"

#include "pdf/PDFHandler.h"

namespace DocantoWin {
	class MainWindowHandler {
		std::shared_ptr<Window> m_mainwindow;
		std::shared_ptr<Direct2DRender> m_render;
		std::shared_ptr<Caption> m_uicaption;

		std::shared_ptr<PDFHandler> m_pdfhandler;

		void paint();
		void size(Docanto::Geometry::Dimension<long> d);
		void key(Window::VK key, bool pressed);
	public:
		MainWindowHandler(HINSTANCE instance);

		void run();

	};
}

#endif // !_DOCANTOWIN_MAINWINDOWHANDLER_H_
