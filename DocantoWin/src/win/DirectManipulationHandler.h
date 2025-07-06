#ifndef _DOCANTOWIN_DMHANDLER_H_
#define _DOCANTOWIN_DMHANDLER_H_

#include "../include.h"
#include <directmanipulation.h>


namespace DocantoWin {
	class Window;
	class DirectManipulationHandler {
		HWND m_attached_window;

		static IDirectManipulationManager* m_manager;

		IDirectManipulationUpdateManager* m_updatemanager = nullptr;
		IDirectManipulationViewport* m_viewport = nullptr;
		DWORD m_eventhandlercookie;

	public:
		DirectManipulationHandler(HWND h);
		~DirectManipulationHandler();
	};
}

#endif // !_DOCANTOWIN_DMHANDLER_H_