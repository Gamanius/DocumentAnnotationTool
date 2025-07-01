#include "Window.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

LRESULT Window::parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Example message handling
	Window* currentInstance = Window::get_instances()[hWnd];
	// The WM_CREATE msg must be translated before anything else
	if (currentInstance == nullptr) {
		if (uMsg == WM_CREATE) {
			// add this windows to the window stack
			auto window = reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams;
			Window::get_instances()[hWnd] = reinterpret_cast<Window*>(window);

			// notify that the frame has changed
			SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
			return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_ENDSESSION:
	case WM_CLOSE:
	{
		currentInstance->m_closeRequest = true;
		return NULL;
	}
	case WM_NCCALCSIZE:
	{
		if (!wParam) return DefWindowProc(hWnd, uMsg, wParam, lParam);
		UINT dpi = GetDpiForWindow(hWnd);
	
		int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
		int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
	
		NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
		RECT* requested_client_rect = params->rgrc;
	
		requested_client_rect->right -= frame_x + padding;
		requested_client_rect->left += frame_x + padding;
		requested_client_rect->bottom -= frame_y + padding;
	
		// we only need to add the padding if the window is maximized
		if (currentInstance->is_window_maximized()) {
			requested_client_rect->top += padding;
		}
	
		return 1;
	}
	case WM_SIZE:
	case WM_SIZING:
	{
		if (currentInstance->m_callback_size) {
			currentInstance->m_callback_size(currentInstance->get_window_size());
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_PAINT:
	{
		currentInstance->m_callback_paint();
		ValidateRect(hWnd, nullptr);
		return 0;
	}
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return true;
}

std::map<HWND, Window*>& Window::get_instances() {
	static std::map<HWND, Window*> instances;
	return instances;
}

Window::Window(HINSTANCE h) {
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
		Docanto::Logger::warn("Could not set DPI awareness");


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
}

Window::~Window() {
	get_instances().erase(m_hwnd);
	if (!DestroyWindow(m_hwnd))
		Docanto::Logger::error("Error when destroying window");
}


UINT Window::get_dpi() const {
	return GetDpiForWindow(m_hwnd);
}

void Window::set_window_title(const std::wstring& s) {
	SetWindowText(m_hwnd, s.c_str());
}

Docanto::Geometry::Dimension<long> Window::get_client_size() const {
	RECT r;
	GetClientRect(m_hwnd, &r);
	return Docanto::Geometry::Dimension<long>(r.right, r.bottom);
}

Docanto::Geometry::Dimension<long> Window::get_window_size() const {
	RECT r;
	DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(RECT));
	return RectToDimension(r);
}

Docanto::Geometry::Point<long> Window::get_window_position() const {
	RECT r;
	DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(RECT));
	return Docanto::Geometry::Point<long>(r.left, r.top);
}

bool Window::is_window_maximized() const {
	return IsZoomed(m_hwnd) != 0;
}

bool Window::get_close_request() const {
	return m_closeRequest;
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

void Window::set_window_size(Docanto::Geometry::Rectangle<long> r) {
	SetWindowPos(m_hwnd, HWND_TOP, r.x, r.y, r.width, r.height, SWP_NOZORDER);
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


void Window::set_callback_paint(std::function<void()> callback) {
	m_callback_paint = callback;
}


void Window::set_callback_size(std::function<void(Docanto::Geometry::Dimension<long>)> callback) {
	m_callback_size = callback;
}