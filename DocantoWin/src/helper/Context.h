#pragma once

#ifndef _DOCANTOWIN_CONTEXT_H_
#define _DOCANTOWIN_CONTEXT_H_

#include <memory>

namespace DocantoWin {
	class UIHandler;
	class Direct2DRender;
	class Caption;
	class Window;
	class TabHandler;

	struct Context {
		std::shared_ptr<Direct2DRender> render;
		std::shared_ptr<Window> window;

		std::shared_ptr<Caption> caption;
		std::shared_ptr<UIHandler> uihandler;

		std::shared_ptr<TabHandler> tabs;
	};
}

#endif // !_DOCANTOWIN_CONTEXT_H_
