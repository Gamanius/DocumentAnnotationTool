#pragma once

#include <Windows.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>


_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __cdecl operator new(size_t _Size);

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __cdecl operator new[](size_t _Size);

void operator delete(void* ptr) noexcept;

void operator delete[](void* ptr) noexcept;

namespace MemoryWatcher {
	size_t get_allocated_bytes();
	size_t get_all_allocated_bytes();
	void set_calibration();
}

namespace Logger {
	enum MsgLevel {
		INFO = 0,
		WARNING = 1,
		FATAL = 2
	};

	void log(const std::wstring& msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const std::string& msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const unsigned long msg, MsgLevel lvl = MsgLevel::INFO);
	void warn(const std::string& msg);

	void assert_msg(const std::string& msg, const std::string& file, long line);
	void assert_msg_win(const std::string& msg, const std::string& file, long line);
	
	void print_to_console(HANDLE handle);

	void clear();

	const std::vector<std::wstring>* get_all_msg();
}


class WindowHandler {
	HWND m_hwnd = NULL;
	HDC m_hdc = NULL;
	UINT m_dpi = 96;

	bool m_closeRequest = false;

	static std::map<HWND, WindowHandler*> m_allWindowInstances;

public:

	enum WINDOW_STATE {
		HIDDEN,
		MAXIMIZED,
		NORMAL,
		MINIMIZED
	};

	struct RenderHandler {

	};

	static LRESULT parseWindowMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Retrieves window messages for _all_ windows
	static void get_window_messages(bool blocking);

	bool init(std::wstring windowname, HINSTANCE instance);

	WindowHandler(std::wstring windowname, HINSTANCE instance);
	~WindowHandler();

	void set_state(WINDOW_STATE state);

	// Returns true if a close request has been sent to the window
	bool close_request();
};

#ifndef NDEBUG
#define ASSERT(x, y) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); }
#define ASSERT_WIN(x,y) if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); }
#define ASSERT_RETURN_FALSE(x,y) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); return false; }
#define ASSERT_WIN_RETURN_FALSE(x,y)  if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); return false; }
#else
#define ASSERT(x, y)
#endif // !NDEBUG
