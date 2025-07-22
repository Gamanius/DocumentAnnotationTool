#pragma once

#ifndef _DOCANTOWIN_UIHANDLER_
#define _DOCANTOWIN_UIHANDLER_


#include "win/Direct2D.h"
#include "UIContainer.h"

namespace DocantoWin {
	class UIHandler : public UIContainer {
	public:
		UIHandler(std::shared_ptr<Direct2DRender> render);
	};
}


#endif // !_DOCANTOWIN_UIHANDLER_