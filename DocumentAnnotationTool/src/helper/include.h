#pragma once

#include <Windows.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <d2d1.h>
#include <dwrite_3.h>

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

inline void SafeRelease(IUnknown* ptr) {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	}
}

namespace Logger {
	enum MsgLevel {
		INFO = 0,
		WARNING = 1,
		FATAL = 2
	};

	void init();

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

namespace Renderer {
	template <typename T>
	struct Point {
		T x, y = 0;

		Point() = default;
		Point(T x, T y) : x(x), y(y) {}
	};

	template <typename T>
	struct Rectangle {
		T x, y = 0;
		T width, height = 0;

		Rectangle() = default;
		Rectangle(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}

		operator D2D1_SIZE_U() const {
			return D2D1::SizeU((UINT32)width, (UINT32)height);
		}

		T right() { return x + width; }
		T bottom() { return y + height; }
	};

	struct Color {
		byte r, g, b = 0;
		Color() = default;
		Color(byte r, byte g, byte b) : r(r), g(g), b(b) {}

		operator D2D1_COLOR_F() const {
			return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f);
		}
	};


	struct RenderHandler {
		RenderHandler() = default;

		virtual void clear(Color c) {}

		virtual void draw_text(std::wstring text, Point<float> topleft, std::wstring font, float size) {}
	};
}

class WindowHandler;

class Direct2DRenderer : public Renderer::RenderHandler {
	static ID2D1Factory* m_factory; 
	static IDWriteFactory3* m_writeFactory;
	WindowHandler* m_attachedWindow = nullptr;
	ID2D1HwndRenderTarget* m_renderTarget = nullptr;

	UINT32 m_isRenderinProgress = 0;

	void begin_draw(); 
	void end_draw();


public:
	struct RenderObject {
		//TODO
	};
	struct TextFormat {
		IDWriteTextFormat* m_format = nullptr;
		ID2D1SolidColorBrush* m_brush = nullptr;

		TextFormat() = default;
		TextFormat(TextFormat&& t) noexcept {
			m_format = t.m_format;
			m_brush = t.m_brush;
			t.m_format = nullptr;
			t.m_brush = nullptr;
		}
		TextFormat(const TextFormat& t) {
			m_format = t.m_format;
			m_brush = t.m_brush;

			if (m_format != nullptr)
				m_format->AddRef();
			if (m_brush != nullptr)
				m_brush->AddRef();
		}
		TextFormat& operator=(const TextFormat& t) {
			if (this != &t) {
				SafeRelease(m_format);
				SafeRelease(m_brush);

				m_format = t.m_format;
				m_brush = t.m_brush;

				if (m_format != nullptr)
					m_format->AddRef();
				if (m_brush != nullptr)
					m_brush->AddRef();
			}
			return *this;
		}
		TextFormat& operator=(TextFormat&& t) noexcept {
			if (this != &t) {
				SafeRelease(m_format);
				SafeRelease(m_brush);

				m_format = t.m_format;
				m_brush = t.m_brush;

				t.m_format = nullptr;
				t.m_brush = nullptr;
			}
			return *this;
		}

		~TextFormat() {
			SafeRelease(m_format);
			SafeRelease(m_brush);
		}
	};

	Direct2DRenderer(WindowHandler* const window);
	~Direct2DRenderer();

	void clear(Renderer::Color c) override;

	void draw_text(std::wstring text, Renderer::Point<float> topleft, std::wstring font, float size) override;

	TextFormat create_text_format(std::wstring font, float size);
};


class WindowHandler {
	HWND m_hwnd = NULL;
	HDC m_hdc = NULL;
	UINT m_dpi = 96;

    Renderer::RenderHandler* m_renderer = nullptr;

	bool m_closeRequest = false;

	static std::map<HWND, WindowHandler*>* m_allWindowInstances;

	static void cleanup();

public:

	enum WINDOW_STATE {
		HIDDEN,
		MAXIMIZED,
		NORMAL,
		MINIMIZED
	};


	static LRESULT parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Retrieves window messages for _all_ windows
	static void get_window_messages(bool blocking);

	bool init(std::wstring windowname, HINSTANCE instance);

	WindowHandler(std::wstring windowname, HINSTANCE instance);
	~WindowHandler();

	void set_state(WINDOW_STATE state);

	void set_renderer(Renderer::RenderHandler* renderer);

	// Returns the window handle
	HWND get_hwnd();

	// Returns the device context
	HDC get_hdc();

	// Returns the DPI of the window
	UINT get_dpi();

	// Returns the window size
	Renderer::Rectangle<long> get_size();

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
