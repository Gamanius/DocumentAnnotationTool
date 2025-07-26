#pragma once

#ifndef _DOCANTOWIN_UIHANDLER_
#define _DOCANTOWIN_UIHANDLER_

#include "helper/Context.h"
#include "UIContainer.h"

namespace DocantoWin {
	class UIHandler : public UIContainer {
	public:
		UIHandler(std::weak_ptr<Context> render);
	};
}


#endif // !_DOCANTOWIN_UIHANDLER_