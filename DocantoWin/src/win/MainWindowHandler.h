#ifndef _DOCANTOWIN_MAINWINDOWHANDLER_H_
#define _DOCANTOWIN_MAINWINDOWHANDLER_H_

#include "Window.h"
#include "Direct2D.h"

#include "ui/Caption.h"
#include "ui/Gesture.h"
#include "ui/UIHandler.h"

#include "pdf/PDFHandler.h"

namespace DocantoWin {
	class MainWindowHandler {
		std::shared_ptr<Window> m_mainwindow;
		std::shared_ptr<Direct2DRender> m_render;
		std::shared_ptr<Caption> m_uicaption;

		std::shared_ptr<GestureHandler> m_gesture;

		std::shared_ptr<PDFHandler> m_pdfhandler;

		std::shared_ptr<UIHandler> m_uihandler;

		void paint();
		void size(Docanto::Geometry::Dimension<long> d);
		void key(Window::VK key, bool pressed);
		void pointer_down(Window::PointerInfo p);
		void pointer_update(Window::PointerInfo p);
		void pointer_up(Window::PointerInfo p);
	public:
		MainWindowHandler(HINSTANCE instance);

		void run();

	};
}

#endif // !_DOCANTOWIN_MAINWINDOWHANDLER_H_
