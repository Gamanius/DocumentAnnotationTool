#ifndef _DOCANTOWIN_MAINWINDOWHANDLER_H_
#define _DOCANTOWIN_MAINWINDOWHANDLER_H_

#include "helper/Context.h"
#include "helper/TabHandler.h"

#include "ui/Gesture.h"
#include "ui/UIHandler.h"

#include "pdf/PDFHandler.h"
#include "pdf/ToolHandler.h"

#include "../../rsc/resource.h"

namespace DocantoWin {
	class MainWindowHandler {
		std::shared_ptr<Context> m_ctx;

		std::shared_ptr<GestureHandler> m_gesture;

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
