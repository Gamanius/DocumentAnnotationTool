#include "Window.h"
#include <dwmapi.h>
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")

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
				PostMessage(m_hwnd, WM_PAINT, 0, 0);
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
