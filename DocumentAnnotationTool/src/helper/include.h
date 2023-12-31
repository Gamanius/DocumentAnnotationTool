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
#include <tuple>
#include <crtdbg.h>
#include <array>
#include "mupdf/fitz.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <mupdf/fitz/crypt.h>


#define APPLICATION_NAME L"Docanto"
#define EPSILON 0.00001f

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
	void log(const double msg, MsgLevel lvl = MsgLevel::INFO);
	void warn(const std::string& msg);

	void assert_msg(const std::string& msg, const std::string& file, long line);
	void assert_msg_win(const std::string& msg, const std::string& file, long line);
	void assert_msg(const std::wstring& msg, const std::string& file, long line);
	void assert_msg_win(const std::wstring& msg, const std::string& file, long line);
	
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
		Point(POINT p) : x(p.x), y(p.y) {}
		template <typename W>
		Point(Point<W> p) : x((T)p.x), y((T)p.y) {}
		Point(D2D1_POINT_2F p) : x((T)p.x), y((T)p.y) {}
		Point(T x, T y) : x(x), y(y) {}

		operator D2D1_POINT_2F() const {
			return D2D1::Point2F(x, y);
		}

		operator std::wstring() const {
			return std::to_wstring(x) + L", " + std::to_wstring(y);
		}



		Point<T> operator +(const Point<T>& p) const { 
			return Point<T>(x + p.x, y + p.y); 
		}
		Point<T> operator -(const Point<T>& p) const {
			return Point<T>(x - p.x, y - p.y);
		}

		Point<T> operator / (const T& f) const {
			return Point<T>(x / f, y / f);
		}

		Point<T> operator *(const T& f) const {
			return Point<T>(x * f, y * f);
		}


		Point<T>& operator /= (const T& f) {
			x /= f;
			y /= f;
			return *this;
		}
		Point<T>& operator +=(const Point<T>& p) { 
			x += p.x;
			y += p.y;

			return *this;
		}
	};

	template <typename T>
	struct Rectangle {
		T x = 0, y = 0;
		T width = 0, height = 0;

		Rectangle() = default;
		Rectangle(T x, T y, T width, T height) : x(x), y(y), width(width), height(height) {}
		Rectangle(Point<T> p, T width, T height) : x(p.x), y(p.y), width(width), height(height) {}
		Rectangle(Point<T> p1, Point<T> p2) : x(p1.x), y(p1.y), width(p2.x - p1.x), height(p2.y - p1.y) {}
		Rectangle(fz_rect rect) : x(rect.x0), y(rect.y0), width(rect.x1 - rect.x0), height(rect.y1 - rect.y0) {}
		Rectangle(fz_irect rect) : x(rect.x0), y(rect.y0), width(rect.x1 - rect.x0), height(rect.y1 - rect.y0) {}

		operator D2D1_SIZE_U() const {
			return D2D1::SizeU((UINT32)width, (UINT32)height);
		}

		T right() const { return x + width; }
		T bottom() const { return y + height; }
		Point<T> upperleft() const { return { x, y }; }

		operator RECT() const { 
			return { x, y, right(), bottom() };
		} 

		operator D2D1_RECT_F() const {
			return { x, y, right(), bottom() };
		}

		operator std::wstring() const {
			return std::to_wstring(x) + L", " + std::to_wstring(y) + L", " + std::to_wstring(width) + L", " + std::to_wstring(height); 
		}

		template <typename W>
		operator Rectangle<W>() const {
			return Rectangle<W>((W)x, (W)y, (W)width, (W)height);
		}

		operator fz_rect() const {
			return { x, y, right(), bottom() };
		}

		bool intersects(const Rectangle<T>& other) const {
			return (x < other.x + other.width && 
				x + width > other.x && 
				y < other.y + other.height && 
				y + height > other.y); 
		}

		Rectangle<T> calculate_overlap(const Rectangle<T>& other) const {
			Rectangle<T> overlap;

			// Calculate overlap in x-axis
			overlap.x = max(x, other.x);
			T x2 = min(x + width, other.x + other.width);
			overlap.width = max(static_cast<T>(0), x2 - overlap.x);

			// Calculate overlap in y-axis
			overlap.y = max(y, other.y);
			T y2 = min(y + height, other.y + other.height);
			overlap.height = max(static_cast<T>(0), y2 - overlap.y);

			return overlap;
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
	
	D2D1::Matrix3x2F m_transformPosMatrix = D2D1::Matrix3x2F::Identity(), m_transformScaleMatrix = D2D1::Matrix3x2F::Identity();
	float m_transformScale = 1.0f;
	Renderer::Point<float> m_transformPos, m_transformScaleCenter;

	std::atomic<UINT32> m_isRenderinProgress = 0;
	std::mutex draw_lock;

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

	enum INTERPOLATION_MODE {
		NEAREST_NEIGHBOR,
		LINEAR
	};

	Direct2DRenderer(const WindowHandler& window);
	~Direct2DRenderer();

	void begin_draw(); 
	void end_draw();
	void clear(Renderer::Color c) override;
	void resize(Renderer::Rectangle<long> r);
	void draw_bitmap(BitmapObject& bitmap, Renderer::Point<float> pos, INTERPOLATION_MODE mode = INTERPOLATION_MODE::NEAREST_NEIGHBOR, float opacity = 1.0f);
	void draw_bitmap(BitmapObject& bitmap, Renderer::Rectangle<float> dest, INTERPOLATION_MODE mode = INTERPOLATION_MODE::NEAREST_NEIGHBOR, float opacity = 1.0f);
	void draw_text(const std::wstring& text, Renderer::Point<float> pos, TextFormatObject& format, BrushObject& brush);
	void draw_rect(Renderer::Rectangle<float> rec, BrushObject& brush, float thick);

	void set_current_transform_active();
	void set_identity_transform_active();

	void set_transform_matrix(Renderer::Point<float> p);
	void add_transform_matrix(Renderer::Point<float> p);
	void set_scale_matrix(float scale, Renderer::Point<float> center);
	void add_scale_matrix(float scale, Renderer::Point<float> center);

	float get_transform_scale() const;
	Renderer::Point<float> get_transform_pos() const;
	Renderer::Rectangle<long> get_window_size() const;
	/// <summary>
	/// Transforms the rectangle using the current transformation matrices
	/// </summary>
	/// <param name="rec">The rectangle to be transformed</param>
	/// <returns>The new transformed rectangle</returns>
	Renderer::Rectangle<float> transform_rect(const Renderer::Rectangle<float> rec) const;

	Renderer::Rectangle<float> inv_transform_rect(const Renderer::Rectangle<float> rec) const;

	UINT get_dpi() const;

	TextFormatObject create_text_format(std::wstring font, float size);
	BrushObject create_brush(Renderer::Color c);
	BitmapObject create_bitmap(const std::wstring& path);
	BitmapObject create_bitmap(const byte* const data, Renderer::Rectangle<unsigned int> size, unsigned int stride, float dpi);
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

			size = t.size;
			t.size = 0;
		}

		// Dont copy data as it could be very expensive
		File(const File& t) = delete;
		File& operator=(const File& t) = delete;

		File& operator=(File&& t) noexcept {
			if (this != &t) {
				data = t.data;
				t.data = nullptr;

				size = t.size;
				t.size = 0;
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

constexpr float MUPDF_DEFAULT_DPI = 72.0f;
class PDFHandler;
class MuPDFHandler {
	fz_context* m_ctx = nullptr;
	std::mutex m_mutex[FZ_LOCK_MAX];
	fz_locks_context m_locks;

	static void lock_mutex(void* user, int lock);
	static void unlock_mutex(void* user, int lock);
	static void error_callback(void* user, const char* message);
public:
	struct PDF : public FileHandler::File {
		fz_document* m_doc = nullptr;
		fz_context* m_ctx = nullptr;
		std::vector<fz_page*> m_pages;

		PDF() = default;
		PDF(fz_context* ctx, fz_document* doc);

		// move constructor
		PDF(PDF&& t) noexcept;
		// move assignment
		PDF& operator=(PDF&& t) noexcept;

		// again dont copy
		PDF(const PDF& t) = delete;
		PDF& operator=(const PDF& t) = delete;

		// the fz_context is not allowed to be droped here
		~PDF();

		fz_page* get_page(size_t page) const;

		/// <summary>
		/// Will render out the pdf page to a bitmap. It will always give back a bitmap with the DPI 96.
		/// To account for higher DPI's it will instead scale the size of the image using <c>get_page_size </c> function
		/// </summary>
		/// <param name="renderer">Reference to the Direct2D renderer</param>
		/// <param name="page">The page to be rendererd</param>
		/// <param name="dpi">The dpi at which it should be rendered</param>
		/// <returns>The bitmap object</returns>
		Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi = 72.0f) const;
		/// <summary>
		/// Will render out the pdf page to a bitmap with the given size
		/// </summary>
		/// <param name="renderer">Reference to the Direct2D renderer</param>
		/// <param name="page">The page to be rendererd</param>
		/// <param name="rec">The end size of the Bitmap</param>
		/// <returns>Bitmap</returns>
		Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<unsigned int> rec) const;
		/// <summary>
		/// Will render out the area of the pdf page given by the source rectangle to a bitmap with the given dpi. 
		/// </summary>
		/// <param name="renderer">Reference to the Direct2D renderer</param>
		/// <param name="page">The page to be rendererd</param>
		/// <param name="source">The area of the page in docpage pixels. Should be smaller than get_page_size() would give</param>
		/// <param name="dpi">The dpi at which it should be rendered</param>
		/// <returns>Returns the DPI-scaled bitmap</returns>
		/// <remark>If the DPI is 72 the size of the returned bitmap is the same as the size of the source</remark>
		Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<float> source, float dpi) const;

		void multithreaded_get_bitmap(Direct2DRenderer* renderer, size_t page, Renderer::Rectangle<float> source, Renderer::Rectangle<float> dest, float dpi, PDFHandler* pdfhandler) const;

		/// <summary>
		/// Retrieves the number of pdf pages
		/// </summary>
		/// <returns>Number of pages</returns>
		size_t get_page_count() const;

		Renderer::Rectangle<float> get_page_size(size_t page, float dpi = 72) const;
	};

	MuPDFHandler();

	std::optional<PDF> load_pdf(const std::wstring& path);

	operator fz_context* () const { return m_ctx; }

	~MuPDFHandler();
};

class PDFHandler {
	MuPDFHandler::PDF m_pdf;
	// not owned by this class!
	Direct2DRenderer* const m_renderer = nullptr;
	// rendererd at half scale or otherwise specified
	std::vector<Direct2DRenderer::BitmapObject> m_previewBitmaps;
	std::vector<Renderer::Rectangle<float>> m_pagerec;

	std::mutex m_draw_lock;

	float m_seperation_distance = 10;
	float m_preview_scale = 0.5; 

	void create_preview(float scale = 0.5);
	void render_high_res();

public:
	PDFHandler() = default;
	/// <summary>
	/// Will load the pdf from the given path. The MuPDF context and the Direct2DRenderer are not owned by this class
	/// </summary>
	/// <param name="renderer">Renderer that should be rendered to</param>
	/// <param name="handler">Mupdf context</param>
	/// <param name="path">path of the pdf</param>
	PDFHandler(Direct2DRenderer* const renderer, MuPDFHandler& handler, const std::wstring& path);

	// move constructor
	PDFHandler(PDFHandler&& t) noexcept;
	// move assignment
	PDFHandler& operator=(PDFHandler&& t) noexcept;

	void render_preview(); 
	void render();
	void sort_page_positions();

	~PDFHandler();
};

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

	enum VK {
		LEFT_MB,
		RIGHT_MB,
		CANCEL,
		MIDDLE_MB,
		X1_MB,
		X2_MB,
		LEFT_SHIFT,
		RIGHT_SHIFT,
		LEFT_CONTROL,
		RIGHT_CONTROL,
		BACKSPACE,
		TAB,
		ENTER,
		ALT,
		PAUSE,
		CAPSLOCK,
		ESCAPE,
		SPACE,
		PAGE_UP,
		PAGE_DOWN,
		END,
		HOME,
		LEFTARROW,
		UPARROW,
		RIGHTARROW,
		DOWNARROW,
		SELECT,
		PRINT,
		EXECUTE,
		PRINT_SCREEN,
		INSERT,
		DEL,
		HELP,
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LEFT_WINDOWS,
		RIGHT_WINDOWS,
		APPLICATION,
		SLEEP,
		SCROLL_LOCK,
		LEFT_MENU,
		RIGHT_MENU,
		VOLUME_MUTE,
		VOLUME_DOWN,
		VOLUME_UP,
		MEDIA_NEXT,
		MEDIA_LAST,
		MEDIA_STOP,
		MEDIA_PLAY_PAUSE,
		OEM_1,
		OEM_2,
		OEM_3,
		OEM_4,
		OEM_5,
		OEM_6,
		OEM_7,
		OEM_8,
		OEM_102,
		OEM_CLEAR,
		OEM_PLUS,
		OEM_COMMA,
		OEM_MINUS,
		OEM_PERIOD,
		NUMPAD_0,
		NUMPAD_1,
		NUMPAD_2,
		NUMPAD_3,
		NUMPAD_4,
		NUMPAD_5,
		NUMPAD_6,
		NUMPAD_7,
		NUMPAD_8,
		NUMPAD_9,
		NUMPAD_MULTIPLY,
		NUMPAD_ADD,
		NUMPAD_SEPERATOR,
		NUMPAD_SUBTRACT,
		NUMPAD_COMMA,
		NUMPAD_DIVIDE,
		NUMPAD_LOCK,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		PLAY,
		ZOOM,
		UNKWON
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
	std::function<void(short, bool, Renderer::Point<int>)> m_callback_mousewheel;
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
	void set_callback_mousewheel(std::function<void(short, bool, Renderer::Point<int>)> callback);

	void invalidate_drawing_area();
	void invalidate_drawing_area(Renderer::Rectangle<long> rec);

	// Returns the window handle
	HWND get_hwnd() const;

	// Returns the device context
	HDC get_hdc() const;

	// Returns the DPI of the window
	UINT get_dpi() const;

	/// <summary>
	/// Converts the given pixel to DIPs (device independent pixels)
	/// </summary>
	/// <param name="px">The pixels</param>
	/// <returns>DIP</returns>
	template <typename T>
	T PxToDp(T px) const {
		return px / (get_dpi() / 96.0f);
	}

	/// <summary>
	/// Converts the DIP to on screen pixels
	/// </summary>
	/// <param name="dip">the dip</param>
	/// <returns>Pixels in screen coordinates</returns>
	template <typename T>
	T DptoPx(T dip) const {
		return dip * (get_dpi() / 96.0f);
	}

	// Returns the window size
	Renderer::Rectangle<long> get_size() const;

	/// <summary>
	/// Returns true if a close request has been sent to the window
	/// </summary>
	/// <returns>true if there has been a close request</returns>
	bool close_request() const;

	static bool is_key_pressed(VK key);

	Renderer::Point<long> get_mouse_pos() const;

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