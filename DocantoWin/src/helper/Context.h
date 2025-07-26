#pragma once

#ifndef _DOCANTOWIN_CONTEXT_H_
#define _DOCANTOWIN_CONTEXT_H_

#include "win/Direct2D.h"
#include "ui/Caption.h"

namespace DocantoWin {
	class UIHandler;

	struct Context {
		std::shared_ptr<Direct2DRender> render;
		std::shared_ptr<Window> window;

		std::shared_ptr<Caption> caption;
		std::shared_ptr<UIHandler> uihandler;

		~Context() {
			Docanto::Logger::log("dewa");
		}
	};
}

#endif // !_DOCANTOWIN_CONTEXT_H_
