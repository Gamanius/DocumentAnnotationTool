#include "Window.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

std::unique_ptr<std::map<HWND, Window*>> Window::m_allWindowInstances;

LRESULT Window::parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Example message handling
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

Window::Window(HINSTANCE h) {
	if (Window::m_allWindowInstances == nullptr) {
		Window::m_allWindowInstances = std::make_unique<std::map<HWND, Window*>>();
		if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
			Docanto::Logger::error("Could not set DPI awereness");
	}


	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = reinterpret_cast<WNDPROC>(this->parse_window_messages);
	wc.hInstance = h;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(3289650);
	wc.lpszClassName = L"Docanto";
	if (!RegisterClass(&wc)) {
		Docanto::Logger::error("Could not register Window");
	}

	m_hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wc.lpszClassName, APPLICATION_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, h, this);
	if (!m_hwnd) {
		Docanto::Logger::error("Window creation was not succefull");
	}

	m_hdc = GetDC(m_hwnd);
	if (!m_hdc) {
		Docanto::Logger::error("Could not retrieve device m_context");
	}

	bool temp = EnableMouseInPointer(true);
	if (!temp) {
		Docanto::Logger::error("Couldn't add Mouse input into Pointer Input Stack API");
	}

	// Does not work but i keep it
	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setuserobjectinformationw
	BOOL b = FALSE;
	auto res = SetUserObjectInformation(GetProcessWindowStation(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &b, sizeof(BOOL));
	//ASSERT_WIN_RETURN_FALSE(res != 0, "Could not set timer exception suppression"); 
}


UINT Window::get_dpi() const {
	return GetDpiForWindow(m_hwnd);
}

void Window::set_window_title(const std::wstring& s) {
	SetWindowText(m_hwnd, s.c_str());
}

Docanto::Geometry::Rectangle<long> Window::get_client_size() const {
	RECT r;
	GetClientRect(m_hwnd, &r);
	return Docanto::Geometry::Rectangle<long>(0, 0, r.right, r.bottom);
}

Docanto::Geometry::Rectangle<long> Window::get_window_size() const {
	RECT r;
	DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(RECT));
	return RectToRectangle(r);
}

Docanto::Geometry::Point<long> Window::get_window_position() const {
	auto r = get_window_size();
	return Docanto::Geometry::Point<long>(r.x, r.y);
}

bool Window::is_window_maximized() const {
	return IsZoomed(m_hwnd) != 0;
}

void Window::set_state(WINDOW_STATE state) {
	int nCmdShow = 0;
	switch (state) {
	case WINDOW_STATE::HIDDEN:    nCmdShow = SW_HIDE;     break;
	case WINDOW_STATE::MAXIMIZED: nCmdShow = SW_MAXIMIZE; break;
	case WINDOW_STATE::NORMAL:    nCmdShow = SW_RESTORE;  break;
	case WINDOW_STATE::MINIMIZED: nCmdShow = SW_MINIMIZE; break;
	}
	ShowWindow(m_hwnd, nCmdShow);
}

void Window::get_window_messages(bool blocking) {
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
