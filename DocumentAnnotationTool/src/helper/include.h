#pragma once

#include <Windows.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <d2d1.h>
#include <dwrite_3.h>
#include <optional>
#include <crtdbg.h>

#define APPLICATION_NAME L"Docanto"

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


	void log(const std::wstring& msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const std::string& msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const unsigned long msg, MsgLevel lvl = MsgLevel::INFO);
	void warn(const std::string& msg);

	void assert_msg(const std::string& msg, const std::string& file, long line);
	void assert_msg_win(const std::string& msg, const std::string& file, long line);
	
	void set_console_handle(HANDLE handle);
	void print_to_console(bool clear = true);

	void print_to_debug(bool clear = true);

	void clear();

	const std::weak_ptr<std::vector<std::wstring>> get_all_msg();
}

namespace Renderer { 
	template <typename T>
	struct Point {
		T x= 0, y = 0;

		Point() = default;
		Point(T x, T y) : x(x), y(y) {}

		operator D2D1_POINT_2F() const {
			return D2D1::Point2F(x, y);
		}
	};

	template <typename T>
	struct Rectangle {
		T x = 0, y = 0;
		T width = 0, height = 0;

		Rectangle() = default;
		Rectangle(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}

		operator D2D1_SIZE_U() const {
			return D2D1::SizeU((UINT32)width, (UINT32)height);
		}

		T right() { return x + width; }
		T bottom() { return y + height; }

		operator std::wstring() const {
			return std::to_wstring(x) + L", " + std::to_wstring(y) + L", " + std::to_wstring(width) + L", " + std::to_wstring(height); 
		}
	};

	struct Color {
		byte r, g, b = 0;
		Color() = default;
		Color(byte r, byte g, byte b) : r(r), g(g), b(b) {}

		operator D2D1_COLOR_F() const {
			return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f);
		}
	};

	struct AlphaColor : public Color {
		byte alpha = 0;
		AlphaColor() = default;
		AlphaColor(byte r, byte g, byte b, byte alpha) : Color(r, g, b), alpha(alpha) {}

		operator D2D1_COLOR_F() const {
			return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, alpha / 255.0f);
		}
	};


	struct RenderHandler {
		RenderHandler() = default;

		virtual void clear(Color c) {}

	};
}

class WindowHandler;

class Direct2DRenderer : public Renderer::RenderHandler {
	static ID2D1Factory* m_factory; 
	static IDWriteFactory3* m_writeFactory;

	HDC m_hdc = nullptr;
	HWND m_hwnd = nullptr;
	Renderer::Rectangle<long> m_window_size;

	ID2D1HwndRenderTarget* m_renderTarget = nullptr;

	UINT32 m_isRenderinProgress = 0;

	void begin_draw(); 
	void end_draw();


public:
	template <typename T>
	struct RenderObject {
		T* m_object = nullptr;

		RenderObject() = default;
		RenderObject(T* object) : m_object(object) {}
		RenderObject(RenderObject&& t) noexcept {
			m_object = t.m_object;
			t.m_object = nullptr;
		}
		RenderObject(const RenderObject& t) {
			m_object = t.m_object;
			m_object->AddRef();
		}

		RenderObject& operator=(const RenderObject& t) {
			if (this != &t) {
				m_object = t.m_object;
				m_object->AddRef();
			}
			return *this;
		}

		RenderObject& operator=(RenderObject&& t) noexcept {
			if (this != &t) {
				m_object = t.m_object;
				t.m_object = nullptr;
			}
			return *this;
		} 

		~RenderObject() {
			SafeRelease(m_object);
		}
	};


	typedef RenderObject<ID2D1SolidColorBrush> BrushObject;
	typedef RenderObject<ID2D1Bitmap> BitmapObject;
	typedef RenderObject<IDWriteTextFormat> TextFormatObject;

	Direct2DRenderer(const WindowHandler& window);
	~Direct2DRenderer();

	void clear(Renderer::Color c) override;
	void resize(Renderer::Rectangle<long> r);
	void draw_bitmap(BitmapObject& bitmap, Renderer::Point<float> pos, float opacity = 1.0f);
	void draw_text(const std::wstring& text, Renderer::Point<float> pos, TextFormatObject& format, BrushObject& brush);

	TextFormatObject create_text_format(std::wstring font, float size);
	BrushObject create_brush(Renderer::Color c);
	BitmapObject create_bitmap(const std::wstring& path);
};

namespace FileHandler {
	struct File {
		byte* data = nullptr;
		size_t size = 0;

		File() = default;
		File(byte* data, size_t size) : data(data), size(size) {}
		File(File&& t) noexcept {
			data = t.data;
			t.data = nullptr;
		}
		
		// Dont copy data as it could be very expensive
		File(const File& t) = delete;
		File& operator=(const File& t) = delete;

		File& operator=(File&& t) noexcept {
			if (this != &t) {
				data = t.data;
				t.data = nullptr;
			}
			return *this;
		}

		~File() {
			delete data;
		}
	};


	/// <summary>
	/// Will create an open file dialog where only *one* file can be selected
	/// </summary>
	/// <param name="filter">A file extension filter. Example "Bitmaps (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0\0"</param>
	/// <param name="windowhandle">Optional windowhandle</param>
	/// <returns>If succeful it will return the path of the file that was selected</returns>
	std::optional<std::wstring> open_file_dialog(const wchar_t* filter, HWND windowhandle = NULL);
	/// <summary>
	/// Will read the contents of the file specified by the path
	/// </summary>
	/// <param name="path">Filepath of the file</param>
	/// <returns>If succefull it will return a file struct with the data</returns>
	std::optional<File> open_file(const std::wstring& path);

	/// <summary>
	/// Returns size of the bitmap
	/// </summary>
	/// <param name="path">Path of the bitmap</param>
	/// <returns>A rectangle with the size</returns>
	std::optional<Renderer::Rectangle<DWORD>> get_bitmap_size(const std::wstring& path);

}


class WindowHandler {
	HWND m_hwnd = NULL;
	HDC m_hdc = NULL;
	UINT m_dpi = 96;

	bool m_closeRequest = false;

	static std::unique_ptr<std::map<HWND, WindowHandler*>> m_allWindowInstances;


public:

	enum WINDOW_STATE {
		HIDDEN,
		MAXIMIZED,
		NORMAL,
		MINIMIZED
	};

	enum POINTER_TYPE {
		UNKNOWN,
		MOUSE,
		STYLUS,
		TOUCH
	};

	struct PointerInfo {
		UINT id = 0;
		POINTER_TYPE type = POINTER_TYPE::UNKNOWN;
		Renderer::Point<float> pos = { 0, 0 };
		UINT32 pressure = 0;
		bool button1pressed = false; /* Left mouse button or barrel button */
		bool button2pressed = false; /* Right mouse button eareser button */
		bool button3pressed = false; /* Middle mouse button */
		bool button4pressed = false; /* X1 Button */
		bool button5pressed = false; /* X2 Button */
	};

private:
	std::function<void()> m_callback_paint;
	std::function<void(Renderer::Rectangle<long>)> m_callback_size;
	std::function<void(PointerInfo)> m_callback_pointer_down;
	std::function<void(PointerInfo)> m_callback_pointer_up;
	std::function<void(PointerInfo)> m_callback_pointer_update;

public:

	static LRESULT parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Retrieves window messages for _all_ windows
	static void get_window_messages(bool blocking);

	bool init(std::wstring windowname, HINSTANCE instance);

	WindowHandler(std::wstring windowname, HINSTANCE instance);
	~WindowHandler();

	void set_state(WINDOW_STATE state);

	void set_callback_paint(std::function<void()> callback);
	void set_callback_size(std::function<void(Renderer::Rectangle<long>)> callback);
	void set_callback_pointer_down(std::function<void(PointerInfo)> callback);
	void set_callback_pointer_up(std::function<void(PointerInfo)> callback);
	void set_callback_pointer_update(std::function<void(PointerInfo)> callback);

	// Returns the window handle
	HWND get_hwnd() const;

	// Returns the device context
	HDC get_hdc() const;

	// Returns the DPI of the window
	UINT get_dpi() const;

	// Returns the window size
	Renderer::Rectangle<long> get_size() const;

	/// <summary>
	/// Returns true if a close request has been sent to the window
	/// </summary>
	/// <returns>true if there has been a close request</returns>
	bool close_request() const;

	/// <summary>
	///  returns the window handle
	/// </summary>
	operator HWND() const { return m_hwnd; }
};

//#ifndef NDEBUG
#define ASSERT(x, y) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); }
#define ASSERT_WIN(x,y) if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); }
#define ASSERT_WITH_STATEMENT(x, y, z) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); z; }
#define ASSERT_WIN_WITH_STATEMENT(x, y, z) if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); z; }
#define ASSERT_WIN_RETURN_FALSE(x,y)  if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); return false; }
#define ASSERT_RETURN_NULLOPT(x,y) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); return std::nullopt; }
#define ASSERT_WIN_RETURN_NULLOPT(x,y)  if (!(x)) { Logger::assert_msg_win(y, __FILE__, __LINE__); __debugbreak(); return std::nullopt; }
//#else
//#define ASSERT(x, y)
//#endif // !NDEBUG