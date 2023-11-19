#pragma once

#include "include.h"
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

std::map<HWND, WindowHandler*>* WindowHandler::m_allWindowInstances;


void WindowHandler::cleanup() {
	delete m_allWindowInstances;
}

LRESULT WindowHandler::parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	WindowHandler* currentInstance = m_allWindowInstances->operator[](hWnd);
	if (currentInstance == nullptr) {
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_CLOSE:
	{
		currentInstance->m_closeRequest = true;
		return NULL;
	}
	case WM_PAINT:
	{
		if (currentInstance->m_renderer != nullptr) {
			currentInstance->m_renderer->clear({50, 50, 50});
		}
	}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void WindowHandler::set_state(WINDOW_STATE state) {
	int nCmdShow = 0;
	switch (state) {
	case WINDOW_STATE::HIDDEN:    nCmdShow = SW_HIDE;     break;
	case WINDOW_STATE::MAXIMIZED: nCmdShow = SW_MAXIMIZE; break;
	case WINDOW_STATE::NORMAL:    nCmdShow = SW_RESTORE;  break;
	case WINDOW_STATE::MINIMIZED: nCmdShow = SW_MINIMIZE; break;
	}
	ShowWindow(m_hwnd, nCmdShow);
}

HWND WindowHandler::get_hwnd() {
	return m_hwnd;
}

void WindowHandler::set_renderer(Renderer::RenderHandler* renderer) {
	m_renderer = renderer;
}

HDC WindowHandler::get_hdc() {
	return m_hdc;
}

UINT WindowHandler::get_dpi() {
	return m_dpi;
}

Renderer::Rectangle<long> WindowHandler::get_size() {
	RECT r;
	GetClientRect(m_hwnd, &r);
	return Renderer::Rectangle<long>(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

bool WindowHandler::close_request() {
	return m_closeRequest;
}

void WindowHandler::get_window_messages(bool blocking) {
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

bool WindowHandler::init(std::wstring windowName, HINSTANCE instance) {
	if (m_allWindowInstances == nullptr) {
		m_allWindowInstances = new std::map<HWND, WindowHandler*>();
		std::atexit(WindowHandler::cleanup);
	}

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = reinterpret_cast<WNDPROC>(this->parse_window_messages); 
	wc.hInstance = instance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(3289650);
	wc.lpszClassName = windowName.c_str();

	ASSERT_WIN_RETURN_FALSE(RegisterClass(&wc), "Could not register Window");

	m_hwnd = CreateWindow(wc.lpszClassName, windowName.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
	ASSERT_WIN_RETURN_FALSE(m_hwnd, "Window creation was not succefull");

	m_hdc = GetDC(m_hwnd);
	ASSERT_WIN_RETURN_FALSE(m_hwnd, "Could not retrieve device context");

	bool temp = EnableMouseInPointer(true);
	ASSERT_WIN_RETURN_FALSE(temp, "Couldn't add Mouse input into Pointer Input Stack API");

	m_dpi = GetDpiForWindow(m_hwnd);
	if (m_dpi == 0) {
		m_dpi = 96;
		Logger::warn("Could not retrieve DPI");
	}
	Logger::log("Retrieved DPI: " + std::to_string(m_dpi));

	// add this windows to the window stack
	m_allWindowInstances->operator[](m_hwnd) = this;

	return true;
}

WindowHandler::WindowHandler(std::wstring windowname, HINSTANCE instance) {
	init(windowname, instance);
}

WindowHandler::~WindowHandler() {
	m_allWindowInstances->erase(m_hwnd);
	ASSERT_WIN(DestroyWindow(m_hwnd), "Error when destroying window");
}
