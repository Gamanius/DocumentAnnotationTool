#include "Window.h"
#include <dwmapi.h>
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")

DocantoWin::Window::VK winkey_to_vk(int windowsKey);

LRESULT DocantoWin::Window::wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Window* me = nullptr;
	if (uMsg == WM_NCCREATE) {
		auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		me = reinterpret_cast<Window*>(cs->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)me);
		me->m_hwnd = hWnd;
	}
	else {
		me = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	if (me) {
		return me->parse_message(uMsg, wParam, lParam);
	}
	else {
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

LRESULT DocantoWin::Window::parse_message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
	{
		SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
	case WM_ENDSESSION:
	case WM_CLOSE:
	{
		m_closeRequest = true;
		return NULL;
	}
	case WM_NCCALCSIZE:
	{
		if (!wParam) return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		UINT dpi = GetDpiForWindow(m_hwnd);

		int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
		int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

		NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
		RECT* requested_client_rect = params->rgrc;

		requested_client_rect->right -= frame_x + padding;
		requested_client_rect->left += frame_x + padding;
		requested_client_rect->bottom -= frame_y + padding;

		// we only need to add the padding if the window is maximized
		if (is_window_maximized()) {
			requested_client_rect->top += padding;
		}

		return 1;
	}
	case WM_NCMOUSEMOVE:
	case WM_NCMOUSELEAVE:
	{
		PostMessage(m_hwnd, WM_PAINT, 0, 0);
		break;
	}
	case WM_NCHITTEST:
	{
		LRESULT hit = DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		switch (hit)
		{
		case HTNOWHERE:
		case HTRIGHT:
		case HTLEFT:
		case HTTOPLEFT:
		case HTTOP:
		case HTTOPRIGHT:
		case HTBOTTOMRIGHT:
		case HTBOTTOM:
		case HTBOTTOMLEFT:
			return hit;
		}

		if (m_callback_nchittest) {

			auto xPos = GET_X_LPARAM(lParam);
			auto yPos = GET_Y_LPARAM(lParam);
			Docanto::Geometry::Point<long> mousepos = { xPos, yPos };
			mousepos = mousepos - get_window_position();

			auto hit = m_callback_nchittest(mousepos);
			
			if (hit != 0) {
				return hit;
			}
		}

		break;
	}
	case WM_NCLBUTTONDOWN:
	{
		switch (wParam) {
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			return 0;
		}
		break;
	}
	case WM_NCLBUTTONUP:
	{
		switch (wParam) {
		case HTMINBUTTON:
			ShowWindow(m_hwnd, SW_MINIMIZE);
			return 0;
		case HTMAXBUTTON:
			ShowWindow(m_hwnd, is_window_maximized() ? SW_RESTORE : SW_MAXIMIZE);
			return 0;
		case HTCLOSE:
			SendMessage(m_hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		break;
	}
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		if (m_callback_key) {
			m_callback_key(winkey_to_vk(wParam), true);
		}

		return 0;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		if (m_callback_key) {
			m_callback_key(winkey_to_vk(wParam), false);
		}

		return 0;
	}
	case WM_SIZE:
	case WM_SIZING:
	{
		if (m_callback_size) {
			m_callback_size(get_client_size());
		}
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
	case WM_PAINT:
	{
		if (m_callback_paint) {
			m_callback_paint();
		}
		ValidateRect(m_hwnd, nullptr);
		return 0;
	}
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}


DocantoWin::Window::Window(HINSTANCE h) {
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
		Docanto::Logger::warn("Could not set DPI awareness");


	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndproc;
	wc.hInstance = h;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(3289650);
	wc.lpszClassName = L"Docanto";
	if (!RegisterClass(&wc)) {
		Docanto::Logger::error("Could not register Window");
	}

	int window_style
		= WS_THICKFRAME   // required for a standard resizeable window
		| WS_SYSMENU      // Explicitly ask for the titlebar to support snapping via Win + <- / Win + ->
		| WS_MAXIMIZEBOX  // Add maximize button to support maximizing via mouse dragging
		// to the top of the screen
		| WS_MINIMIZEBOX;  // Add minimize button to support minimizing by clicking on the taskbar icon

	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, wc.lpszClassName, APPLICATION_NAME,
		window_style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, h, this);

	if (!m_hwnd) {
		Docanto::Logger::error("Window creation was not succefull");
		return;
	}

	m_hdc = GetDC(m_hwnd);
	if (!m_hdc) {
		Docanto::Logger::error("Could not retrieve device m_context");
		return;
	}
	
	bool temp = EnableMouseInPointer(true);
	if (!temp) {
		Docanto::Logger::error("Couldn't add Mouse input into Pointer Input Stack API");
		return;
	}

	Docanto::Logger::success("Initialized window!");
}

DocantoWin::Window::~Window() {
	if (!DestroyWindow(m_hwnd))
		Docanto::Logger::error("Error when destroying window");
}


UINT DocantoWin::Window::get_dpi() const {
	return GetDpiForWindow(m_hwnd);
}

void DocantoWin::Window::set_window_title(const std::wstring& s) {
	SetWindowText(m_hwnd, s.c_str());
}

Docanto::Geometry::Dimension<long> DocantoWin::Window::get_client_size() const {
	RECT r;
	GetClientRect(m_hwnd, &r);
	return Docanto::Geometry::Dimension<long>(r.right, r.bottom);
}

Docanto::Geometry::Dimension<long> DocantoWin::Window::get_window_size() const {
	RECT r;
	DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(RECT));
	return RectToDimension(r);
}

Docanto::Geometry::Point<long> DocantoWin::Window::get_window_position() const {
	RECT r;
	DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(RECT));
	return Docanto::Geometry::Point<long>(r.left, r.top);
}

Docanto::Geometry::Point<long> DocantoWin::Window::get_mouse_pos() const {
	POINT pt;
	if (!GetCursorPos(&pt)) {
		Docanto::Logger::error("Could not retrieve mouse position");
		return { -1, -1 };
	}

	if (!ScreenToClient(m_hwnd, &pt)) {
		Docanto::Logger::error("Could not retrieve mouse position");
		return { -1, -1 };
	}

	return { pt.x, pt.y };
}

bool DocantoWin::Window::is_window_maximized() const {
	return IsZoomed(m_hwnd) != 0;
}

bool DocantoWin::Window::get_close_request() const {
	return m_closeRequest;
}

void DocantoWin::Window::send_close_request() {
	SendMessage(m_hwnd, WM_CLOSE, 0, 0);
}

void DocantoWin::Window::send_paint_request() {
	InvalidateRect(m_hwnd, nullptr, true);
}

void DocantoWin::Window::set_state(WINDOW_STATE state) {
	int nCmdShow = 0;
	switch (state) {
	case WINDOW_STATE::HIDDEN:    nCmdShow = SW_HIDE;     break;
	case WINDOW_STATE::MAXIMIZED: nCmdShow = SW_MAXIMIZE; break;
	case WINDOW_STATE::NORMAL:    nCmdShow = SW_RESTORE;  break;
	case WINDOW_STATE::MINIMIZED: nCmdShow = SW_MINIMIZE; break;
	}
	ShowWindow(m_hwnd, nCmdShow);
}

void DocantoWin::Window::set_window_size(Docanto::Geometry::Rectangle<long> r) {
	SetWindowPos(m_hwnd, HWND_TOP, r.x, r.y, r.width, r.height, SWP_NOZORDER);
}

void DocantoWin::Window::get_window_messages(bool blocking) {
	MSG msg;
	BOOL result;

	//first block
	if (blocking)
		result = GetMessage(&msg, 0, 0, 0);
	else
		result = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);

	while (result != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		//now just peek messages
		result = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
	}
}


void DocantoWin::Window::set_callback_paint(std::function<void()> callback) {
	m_callback_paint = callback;
}


void DocantoWin::Window::set_callback_size(std::function<void(Docanto::Geometry::Dimension<long>)> callback) {
	m_callback_size = callback;
}

void DocantoWin::Window::set_callback_nchittest(std::function<int(Docanto::Geometry::Point<long>)> callback) {
	m_callback_nchittest = callback;
}

void DocantoWin::Window::set_callback_key(std::function<void(VK, bool)> callback) {
	m_callback_key = callback;
}

DocantoWin::Window::VK winkey_to_vk(int windowsKey) {
	switch (windowsKey) {
	case VK_LBUTTON:                           return DocantoWin::Window::VK::LEFT_MB;
	case VK_RBUTTON:		                   return DocantoWin::Window::VK::RIGHT_MB;
	case VK_CANCEL:			                   return DocantoWin::Window::VK::CANCEL;
	case VK_MBUTTON:		                   return DocantoWin::Window::VK::MIDDLE_MB;
	case VK_XBUTTON1:		                   return DocantoWin::Window::VK::X1_MB;
	case VK_XBUTTON2:		                   return DocantoWin::Window::VK::X2_MB;
	case VK_LSHIFT:			                   return DocantoWin::Window::VK::LEFT_SHIFT;
	case VK_RSHIFT:			                   return DocantoWin::Window::VK::RIGHT_SHIFT;
	case VK_LCONTROL:		                   return DocantoWin::Window::VK::LEFT_CONTROL;
	case VK_RCONTROL:		                   return DocantoWin::Window::VK::RIGHT_CONTROL;
	case VK_BACK:			                   return DocantoWin::Window::VK::BACKSPACE;
	case VK_TAB:			                   return DocantoWin::Window::VK::TAB;
	case VK_RETURN:			                   return DocantoWin::Window::VK::ENTER;
	case VK_MENU:			                   return DocantoWin::Window::VK::ALT;
	case VK_PAUSE:			                   return DocantoWin::Window::VK::PAUSE;
	case VK_CAPITAL:		                   return DocantoWin::Window::VK::CAPSLOCK;
	case VK_ESCAPE:			                   return DocantoWin::Window::VK::ESCAPE;
	case VK_SPACE:			                   return DocantoWin::Window::VK::SPACE;
	case VK_PRIOR:			                   return DocantoWin::Window::VK::PAGE_UP;
	case VK_NEXT:			                   return DocantoWin::Window::VK::PAGE_DOWN;
	case VK_END:			                   return DocantoWin::Window::VK::END;
	case VK_HOME:			                   return DocantoWin::Window::VK::HOME;
	case VK_LEFT:			                   return DocantoWin::Window::VK::LEFTARROW;
	case VK_UP:				                   return DocantoWin::Window::VK::UPARROW;
	case VK_RIGHT:			                   return DocantoWin::Window::VK::RIGHTARROW;
	case VK_DOWN:			                   return DocantoWin::Window::VK::DOWNARROW;
	case VK_SELECT:			                   return DocantoWin::Window::VK::SELECT;
	case VK_PRINT:			                   return DocantoWin::Window::VK::PRINT;
	case VK_EXECUTE:		                   return DocantoWin::Window::VK::EXECUTE;
	case VK_SNAPSHOT:		                   return DocantoWin::Window::VK::PRINT_SCREEN;
	case VK_INSERT:			                   return DocantoWin::Window::VK::INSERT;
	case VK_DELETE:			                   return DocantoWin::Window::VK::DEL;
	case VK_HELP:			                   return DocantoWin::Window::VK::HELP;
	case 0x30:				                   return DocantoWin::Window::VK::KEY_0;
	case 0x31:				                   return DocantoWin::Window::VK::KEY_1;
	case 0x32:				                   return DocantoWin::Window::VK::KEY_2;
	case 0x33:				                   return DocantoWin::Window::VK::KEY_3;
	case 0x34:				                   return DocantoWin::Window::VK::KEY_4;
	case 0x35:				                   return DocantoWin::Window::VK::KEY_5;
	case 0x36:				                   return DocantoWin::Window::VK::KEY_6;
	case 0x37:				                   return DocantoWin::Window::VK::KEY_7;
	case 0x38:				                   return DocantoWin::Window::VK::KEY_8;
	case 0x39:				                   return DocantoWin::Window::VK::KEY_9;
	case 0x41:				                   return DocantoWin::Window::VK::A;
	case 0x42:				                   return DocantoWin::Window::VK::B;
	case 0x43:				                   return DocantoWin::Window::VK::C;
	case 0x44:				                   return DocantoWin::Window::VK::D;
	case 0x45:				                   return DocantoWin::Window::VK::E;
	case 0x46:				                   return DocantoWin::Window::VK::F;
	case 0x47:				                   return DocantoWin::Window::VK::G;
	case 0x48:				                   return DocantoWin::Window::VK::H;
	case 0x49:				                   return DocantoWin::Window::VK::I;
	case 0x4a:				                   return DocantoWin::Window::VK::J;
	case 0x4b:				                   return DocantoWin::Window::VK::K;
	case 0x4c:				                   return DocantoWin::Window::VK::L;
	case 0x4d:				                   return DocantoWin::Window::VK::M;
	case 0x4e:				                   return DocantoWin::Window::VK::N;
	case 0x4f:				                   return DocantoWin::Window::VK::O;
	case 0x50:				                   return DocantoWin::Window::VK::P;
	case 0x51:				                   return DocantoWin::Window::VK::Q;
	case 0x52:				                   return DocantoWin::Window::VK::R;
	case 0x53:				                   return DocantoWin::Window::VK::S;
	case 0x54:				                   return DocantoWin::Window::VK::T;
	case 0x55:				                   return DocantoWin::Window::VK::U;
	case 0x56:				                   return DocantoWin::Window::VK::V;
	case 0x57:				                   return DocantoWin::Window::VK::W;
	case 0x58:				                   return DocantoWin::Window::VK::X;
	case 0x59:				                   return DocantoWin::Window::VK::Y;
	case 0x5a:				                   return DocantoWin::Window::VK::Z;
	case VK_LWIN:			                   return DocantoWin::Window::VK::LEFT_WINDOWS;
	case VK_RWIN:			                   return DocantoWin::Window::VK::RIGHT_WINDOWS;
	case VK_APPS:			                   return DocantoWin::Window::VK::APPLICATION;
	case VK_SLEEP:			                   return DocantoWin::Window::VK::SLEEP;
	case VK_SCROLL:			                   return DocantoWin::Window::VK::SCROLL_LOCK;
	case VK_LMENU:			                   return DocantoWin::Window::VK::LEFT_MENU;
	case VK_RMENU:			                   return DocantoWin::Window::VK::RIGHT_MENU;
	case VK_VOLUME_MUTE:	                   return DocantoWin::Window::VK::VOLUME_MUTE;
	case VK_VOLUME_DOWN:	                   return DocantoWin::Window::VK::VOLUME_DOWN;
	case VK_VOLUME_UP:		                   return DocantoWin::Window::VK::VOLUME_UP;
	case VK_MEDIA_NEXT_TRACK:                  return DocantoWin::Window::VK::MEDIA_NEXT;
	case VK_MEDIA_PREV_TRACK:                  return DocantoWin::Window::VK::MEDIA_LAST;
	case VK_MEDIA_STOP:		                   return DocantoWin::Window::VK::MEDIA_STOP;
	case VK_MEDIA_PLAY_PAUSE:                  return DocantoWin::Window::VK::MEDIA_PLAY_PAUSE;
	case VK_OEM_1:			                   return DocantoWin::Window::VK::OEM_1;
	case VK_OEM_2:			                   return DocantoWin::Window::VK::OEM_2;
	case VK_OEM_3:			                   return DocantoWin::Window::VK::OEM_3;
	case VK_OEM_4:			                   return DocantoWin::Window::VK::OEM_4;
	case VK_OEM_5:			                   return DocantoWin::Window::VK::OEM_5;
	case VK_OEM_6:			                   return DocantoWin::Window::VK::OEM_6;
	case VK_OEM_7:			                   return DocantoWin::Window::VK::OEM_7;
	case VK_OEM_8:			                   return DocantoWin::Window::VK::OEM_8;
	case VK_OEM_102:                           return DocantoWin::Window::VK::OEM_102;
	case VK_OEM_CLEAR:		                   return DocantoWin::Window::VK::OEM_CLEAR;
	case VK_OEM_PLUS:		                   return DocantoWin::Window::VK::OEM_PLUS;
	case VK_OEM_COMMA:		                   return DocantoWin::Window::VK::OEM_COMMA;
	case VK_OEM_MINUS:		                   return DocantoWin::Window::VK::OEM_MINUS;
	case VK_OEM_PERIOD:		                   return DocantoWin::Window::VK::OEM_PERIOD;
	case VK_NUMPAD0:		                   return DocantoWin::Window::VK::NUMPAD_0;
	case VK_NUMPAD1:		                   return DocantoWin::Window::VK::NUMPAD_1;
	case VK_NUMPAD2:		                   return DocantoWin::Window::VK::NUMPAD_2;
	case VK_NUMPAD3:		                   return DocantoWin::Window::VK::NUMPAD_3;
	case VK_NUMPAD4:		                   return DocantoWin::Window::VK::NUMPAD_4;
	case VK_NUMPAD5:		                   return DocantoWin::Window::VK::NUMPAD_5;
	case VK_NUMPAD6:		                   return DocantoWin::Window::VK::NUMPAD_6;
	case VK_NUMPAD7:		                   return DocantoWin::Window::VK::NUMPAD_7;
	case VK_NUMPAD8:		                   return DocantoWin::Window::VK::NUMPAD_8;
	case VK_NUMPAD9:		                   return DocantoWin::Window::VK::NUMPAD_9;
	case VK_MULTIPLY:		                   return DocantoWin::Window::VK::NUMPAD_MULTIPLY;
	case VK_ADD:			                   return DocantoWin::Window::VK::NUMPAD_ADD;
	case VK_SEPARATOR:		                   return DocantoWin::Window::VK::NUMPAD_SEPERATOR;
	case VK_SUBTRACT:		                   return DocantoWin::Window::VK::NUMPAD_SUBTRACT;
	case VK_DECIMAL:		                   return DocantoWin::Window::VK::NUMPAD_COMMA;
	case VK_DIVIDE:			                   return DocantoWin::Window::VK::NUMPAD_DIVIDE;
	case VK_NUMLOCK:		                   return DocantoWin::Window::VK::NUMPAD_LOCK;
	case VK_F1:				                   return DocantoWin::Window::VK::F1;
	case VK_F2:				                   return DocantoWin::Window::VK::F2;
	case VK_F3:				                   return DocantoWin::Window::VK::F3;
	case VK_F4:				                   return DocantoWin::Window::VK::F4;
	case VK_F5:				                   return DocantoWin::Window::VK::F5;
	case VK_F6:				                   return DocantoWin::Window::VK::F6;
	case VK_F7:				                   return DocantoWin::Window::VK::F7;
	case VK_F8:				                   return DocantoWin::Window::VK::F8;
	case VK_F9:				                   return DocantoWin::Window::VK::F9;
	case VK_F10:			                   return DocantoWin::Window::VK::F10;
	case VK_F11:			                   return DocantoWin::Window::VK::F11;
	case VK_F12:			                   return DocantoWin::Window::VK::F12;
	case VK_F13:			                   return DocantoWin::Window::VK::F13;
	case VK_F14:			                   return DocantoWin::Window::VK::F14;
	case VK_F15:			                   return DocantoWin::Window::VK::F15;
	case VK_F16:			                   return DocantoWin::Window::VK::F16;
	case VK_F17:			                   return DocantoWin::Window::VK::F17;
	case VK_F18:			                   return DocantoWin::Window::VK::F18;
	case VK_F19:			                   return DocantoWin::Window::VK::F19;
	case VK_F20:			                   return DocantoWin::Window::VK::F20;
	case VK_F21:			                   return DocantoWin::Window::VK::F21;
	case VK_F22:			                   return DocantoWin::Window::VK::F22;
	case VK_F23:			                   return DocantoWin::Window::VK::F23;
	case VK_F24:			                   return DocantoWin::Window::VK::F24;
	case VK_PLAY:			                   return DocantoWin::Window::VK::PLAY;
	case VK_ZOOM: 			                   return DocantoWin::Window::VK::ZOOM;
	default:				                   return DocantoWin::Window::VK::UNKWON;
	}
}


static int vk_to_winkey(DocantoWin::Window::VK key) {
	switch (key) {
	case DocantoWin::Window::VK::LEFT_MB:			   return VK_LBUTTON;
	case DocantoWin::Window::VK::RIGHT_MB:		       return VK_RBUTTON;
	case DocantoWin::Window::VK::CANCEL:				   return VK_CANCEL;
	case DocantoWin::Window::VK::MIDDLE_MB:			   return VK_MBUTTON;
	case DocantoWin::Window::VK::X1_MB:				   return VK_XBUTTON1;
	case DocantoWin::Window::VK::X2_MB:				   return VK_XBUTTON2;
	case DocantoWin::Window::VK::LEFT_SHIFT:			   return VK_LSHIFT;
	case DocantoWin::Window::VK::RIGHT_SHIFT:	       return VK_RSHIFT;
	case DocantoWin::Window::VK::LEFT_CONTROL:	       return VK_LCONTROL;
	case DocantoWin::Window::VK::RIGHT_CONTROL:         return VK_RCONTROL;
	case DocantoWin::Window::VK::BACKSPACE:	           return VK_BACK;
	case DocantoWin::Window::VK::TAB:			       return VK_TAB;
	case DocantoWin::Window::VK::ENTER:	               return VK_RETURN;
	case DocantoWin::Window::VK::ALT:			       return VK_MENU;
	case DocantoWin::Window::VK::PAUSE:    	           return VK_PAUSE;
	case DocantoWin::Window::VK::CAPSLOCK:		       return VK_CAPITAL;
	case DocantoWin::Window::VK::ESCAPE:		           return VK_ESCAPE;
	case DocantoWin::Window::VK::SPACE:		           return VK_SPACE;
	case DocantoWin::Window::VK::PAGE_UP:		       return VK_PRIOR;
	case DocantoWin::Window::VK::PAGE_DOWN:	           return VK_NEXT;
	case DocantoWin::Window::VK::END:			       return VK_END;
	case DocantoWin::Window::VK::HOME:			       return VK_HOME;
	case DocantoWin::Window::VK::LEFTARROW:	           return VK_LEFT;
	case DocantoWin::Window::VK::UPARROW:		       return VK_UP;
	case DocantoWin::Window::VK::RIGHTARROW:	           return VK_RIGHT;
	case DocantoWin::Window::VK::DOWNARROW:	           return VK_DOWN;
	case DocantoWin::Window::VK::SELECT:		           return VK_SELECT;
	case DocantoWin::Window::VK::PRINT:	               return VK_PRINT;
	case DocantoWin::Window::VK::EXECUTE:		       return VK_EXECUTE;
	case DocantoWin::Window::VK::PRINT_SCREEN:	       return VK_SNAPSHOT;
	case DocantoWin::Window::VK::INSERT:		           return VK_INSERT;
	case DocantoWin::Window::VK::DEL:			       return VK_DELETE;
	case DocantoWin::Window::VK::HELP:			       return VK_HELP;
	case DocantoWin::Window::VK::KEY_0:			       return 0x30;
	case DocantoWin::Window::VK::KEY_1:			       return 0x31;
	case DocantoWin::Window::VK::KEY_2:			       return 0x32;
	case DocantoWin::Window::VK::KEY_3:			       return 0x33;
	case DocantoWin::Window::VK::KEY_4:			       return 0x34;
	case DocantoWin::Window::VK::KEY_5:			       return 0x35;
	case DocantoWin::Window::VK::KEY_6:			       return 0x36;
	case DocantoWin::Window::VK::KEY_7:			       return 0x37;
	case DocantoWin::Window::VK::KEY_8:			       return 0x38;
	case DocantoWin::Window::VK::KEY_9:			       return 0x39;
	case DocantoWin::Window::VK::A:				       return 0x41;
	case DocantoWin::Window::VK::B:				       return 0x42;
	case DocantoWin::Window::VK::C:				       return 0x43;
	case DocantoWin::Window::VK::D:				       return 0x44;
	case DocantoWin::Window::VK::E:				       return 0x45;
	case DocantoWin::Window::VK::F:				       return 0x46;
	case DocantoWin::Window::VK::G:				       return 0x47;
	case DocantoWin::Window::VK::H:				       return 0x48;
	case DocantoWin::Window::VK::I:				       return 0x49;
	case DocantoWin::Window::VK::J:				       return 0x4a;
	case DocantoWin::Window::VK::K:				       return 0x4b;
	case DocantoWin::Window::VK::L:				       return 0x4c;
	case DocantoWin::Window::VK::M:				       return 0x4d;
	case DocantoWin::Window::VK::N:				       return 0x4e;
	case DocantoWin::Window::VK::O:				       return 0x4f;
	case DocantoWin::Window::VK::P:				       return 0x50;
	case DocantoWin::Window::VK::Q:				       return 0x51;
	case DocantoWin::Window::VK::R:				       return 0x52;
	case DocantoWin::Window::VK::S:				       return 0x53;
	case DocantoWin::Window::VK::T:				       return 0x54;
	case DocantoWin::Window::VK::U:				       return 0x55;
	case DocantoWin::Window::VK::V:				       return 0x56;
	case DocantoWin::Window::VK::W:				       return 0x57;
	case DocantoWin::Window::VK::X:				       return 0x58;
	case DocantoWin::Window::VK::Y:				       return 0x59;
	case DocantoWin::Window::VK::Z:				       return 0x5a;
	case DocantoWin::Window::VK::LEFT_WINDOWS:	       return VK_LWIN;
	case DocantoWin::Window::VK::RIGHT_WINDOWS:	       return VK_RWIN;
	case DocantoWin::Window::VK::APPLICATION:	       return VK_APPS;
	case DocantoWin::Window::VK::SLEEP:			       return VK_SLEEP;
	case DocantoWin::Window::VK::SCROLL_LOCK:	       return VK_SCROLL;
	case DocantoWin::Window::VK::LEFT_MENU:		       return VK_LMENU;
	case DocantoWin::Window::VK::RIGHT_MENU:	           return VK_RMENU;
	case DocantoWin::Window::VK::VOLUME_MUTE:	       return VK_VOLUME_MUTE;
	case DocantoWin::Window::VK::VOLUME_DOWN:	       return VK_VOLUME_DOWN;
	case DocantoWin::Window::VK::VOLUME_UP:		       return VK_VOLUME_UP;
	case DocantoWin::Window::VK::MEDIA_NEXT:	           return VK_MEDIA_NEXT_TRACK;
	case DocantoWin::Window::VK::MEDIA_LAST:	           return VK_MEDIA_PREV_TRACK;
	case DocantoWin::Window::VK::MEDIA_STOP:	           return VK_MEDIA_STOP;
	case DocantoWin::Window::VK::MEDIA_PLAY_PAUSE:      return VK_MEDIA_PLAY_PAUSE;
	case DocantoWin::Window::VK::OEM_1:				   return VK_OEM_1;
	case DocantoWin::Window::VK::OEM_2:				   return VK_OEM_2;
	case DocantoWin::Window::VK::OEM_3:				   return VK_OEM_3;
	case DocantoWin::Window::VK::OEM_4:				   return VK_OEM_4;
	case DocantoWin::Window::VK::OEM_5:				   return VK_OEM_5;
	case DocantoWin::Window::VK::OEM_6:				   return VK_OEM_6;
	case DocantoWin::Window::VK::OEM_7:				   return VK_OEM_7;
	case DocantoWin::Window::VK::OEM_8:				   return VK_OEM_8;
	case DocantoWin::Window::VK::OEM_102:               return VK_OEM_102;
	case DocantoWin::Window::VK::OEM_CLEAR:			   return VK_OEM_CLEAR;
	case DocantoWin::Window::VK::OEM_PLUS:		       return VK_OEM_PLUS;
	case DocantoWin::Window::VK::OEM_COMMA:			   return VK_OEM_COMMA;
	case DocantoWin::Window::VK::OEM_MINUS:			   return VK_OEM_MINUS;
	case DocantoWin::Window::VK::OEM_PERIOD:	           return VK_OEM_PERIOD;
	case DocantoWin::Window::VK::NUMPAD_0:		       return VK_NUMPAD0;
	case DocantoWin::Window::VK::NUMPAD_1:		       return VK_NUMPAD1;
	case DocantoWin::Window::VK::NUMPAD_2:		       return VK_NUMPAD2;
	case DocantoWin::Window::VK::NUMPAD_3:		       return VK_NUMPAD3;
	case DocantoWin::Window::VK::NUMPAD_4:		       return VK_NUMPAD4;
	case DocantoWin::Window::VK::NUMPAD_5:		       return VK_NUMPAD5;
	case DocantoWin::Window::VK::NUMPAD_6:		       return VK_NUMPAD6;
	case DocantoWin::Window::VK::NUMPAD_7:		       return VK_NUMPAD7;
	case DocantoWin::Window::VK::NUMPAD_8:		       return VK_NUMPAD8;
	case DocantoWin::Window::VK::NUMPAD_9:		       return VK_NUMPAD9;
	case DocantoWin::Window::VK::NUMPAD_MULTIPLY:       return VK_MULTIPLY;
	case DocantoWin::Window::VK::NUMPAD_ADD:	           return VK_ADD;
	case DocantoWin::Window::VK::NUMPAD_SEPERATOR:      return VK_SEPARATOR;
	case DocantoWin::Window::VK::NUMPAD_SUBTRACT:       return VK_SUBTRACT;
	case DocantoWin::Window::VK::NUMPAD_COMMA:	       return VK_DECIMAL;
	case DocantoWin::Window::VK::NUMPAD_DIVIDE:		   return VK_DIVIDE;
	case DocantoWin::Window::VK::NUMPAD_LOCK:	       return VK_NUMLOCK;
	case DocantoWin::Window::VK::F1:					   return VK_F1;
	case DocantoWin::Window::VK::F2:					   return VK_F2;
	case DocantoWin::Window::VK::F3:					   return VK_F3;
	case DocantoWin::Window::VK::F4:					   return VK_F4;
	case DocantoWin::Window::VK::F5:					   return VK_F5;
	case DocantoWin::Window::VK::F6:					   return VK_F6;
	case DocantoWin::Window::VK::F7:					   return VK_F7;
	case DocantoWin::Window::VK::F8:					   return VK_F8;
	case DocantoWin::Window::VK::F9:					   return VK_F9;
	case DocantoWin::Window::VK::F10:			       return VK_F10;
	case DocantoWin::Window::VK::F11:			       return VK_F11;
	case DocantoWin::Window::VK::F12:			       return VK_F12;
	case DocantoWin::Window::VK::F13:			       return VK_F13;
	case DocantoWin::Window::VK::F14:			       return VK_F14;
	case DocantoWin::Window::VK::F15:			       return VK_F15;
	case DocantoWin::Window::VK::F16:			       return VK_F16;
	case DocantoWin::Window::VK::F17:			       return VK_F17;
	case DocantoWin::Window::VK::F18:			       return VK_F18;
	case DocantoWin::Window::VK::F19:			       return VK_F19;
	case DocantoWin::Window::VK::F20:			       return VK_F20;
	case DocantoWin::Window::VK::F21:			       return VK_F21;
	case DocantoWin::Window::VK::F22:			       return VK_F22;
	case DocantoWin::Window::VK::F23:			       return VK_F23;
	case DocantoWin::Window::VK::F24:			       return VK_F24;
	case DocantoWin::Window::VK::PLAY:			       return VK_PLAY;
	case DocantoWin::Window::VK::ZOOM:			       return VK_ZOOM;
	default:									   return -1;
	}
}

bool DocantoWin::Window::is_key_pressed(VK key) {
	return GetKeyState(vk_to_winkey(key)) & 0x8000;

}