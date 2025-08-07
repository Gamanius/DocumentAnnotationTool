#ifndef _DOCANTOWIN_WINDOW_H_
#define _DOCANTOWIN_WINDOW_H_

#include "general/Timer.h"

namespace DocantoWin {
	class Direct2DRender;

	class Window {
		HWND m_hwnd = NULL;
		HDC m_hdc = NULL;

		bool m_closeRequest = false;

		static bool g_touchpadGestureInProgress;

		Docanto::Timer m_last_paint;
		bool m_paint_timer_active = false;

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
			TOUCH,
			TOUCHPAD
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

		enum CURSOR_TYPE {
			NONE,
			POINTER,
			NWSE_RESIZE,
			NESW_RESIZE,
			TOPBOTTOM_RESIZE,
			HAND,
			HAND_GRABBING,
		};

		struct PointerInfo {
			UINT id = 0;
			POINTER_TYPE type = POINTER_TYPE::UNKNOWN;
			Docanto::Geometry::Point<float> pos = { 0, 0 };
			UINT32 pressure = 0;
			bool button1pressed = false; /* Left mouse button or barrel button */
			bool button2pressed = false; /* Right mouse button eareser button */
			bool button3pressed = false; /* Middle mouse button */
			bool button4pressed = false; /* X1 Button */
			bool button5pressed = false; /* X2 Button */
		};

	private:
			std::function<void()> m_callback_paint;
			std::function<void(Docanto::Geometry::Dimension<long>)> m_callback_size;
			std::function<void(Docanto::Geometry::Point<long>)> m_callback_moving;
			std::function<int(Docanto::Geometry::Point<long>)> m_callback_nchittest;
			std::function<void(VK, bool)>  m_callback_key;

			std::function<void(PointerInfo)> m_callback_pointer_down;
			std::function<void(PointerInfo)> m_callback_pointer_up;
			std::function<void(PointerInfo)> m_callback_pointer_update;
			std::function<void(short, bool)> m_callback_mousewheel;
			std::function<CURSOR_TYPE(Docanto::Geometry::Point<long>)>     m_callback_set_cursor;

			std::function<void(Docanto::Geometry::Point<long>, int)> m_callback_pointer_down_nchittest;
			std::function<void(unsigned int)> m_callback_dpi_changed;

			Docanto::Geometry::Dimension<long> m_min_window_size = { 300, 300 };

			bool m_override_default_hittest = false;

			std::optional<CURSOR_TYPE> m_globalcursor;

	public:

		static LRESULT wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT parse_message(UINT uMsg, WPARAM wParam, LPARAM lParam);

		// Retrieves window messages for _all_ windows
		static void get_window_messages(bool blocking);

		Window(HINSTANCE h);
		Window(HINSTANCE h, const std::wstring& name, bool resizable = true);

		Window(Window&& other) noexcept = delete;
		Window& operator=(Window&& other) noexcept = delete;
		Window(const Window& other) = delete;
		Window& operator=(Window& other) = delete;
		~Window();


		// Returns the DPI of the window
		UINT get_dpi() const;
		HWND get_hwnd() const;

		void set_window_title(const std::wstring& s);
		void set_window_style(int style);
		void override_default_hittest(bool a = true);

		// Returns the window size
		Docanto::Geometry::Dimension<long> get_client_size() const;
		Docanto::Geometry::Rectangle<long> get_window_rect() const;
		Docanto::Geometry::Dimension<long> get_window_size() const;
		Docanto::Geometry::Point<long> get_window_position() const;
		Docanto::Geometry::Point<long> get_mouse_pos()       const;

		bool is_window_maximized() const;

		static Docanto::Color get_accent_color();
		static bool is_dark_mode_on();

		void set_min_dims(Docanto::Geometry::Dimension<long> dim);

		// TODO maybe add a callback. for now it just calls WM_PAINT upon change of the theme
		void dark_mode_callback();

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
		T DpToPx(T dip) const {
			return dip * (get_dpi() / 96.0f);
		}


		template<typename T>
		Docanto::Geometry::Point<T> PxToDp(const Docanto::Geometry::Point<T>& pxPoint) const {
			Docanto::Geometry::Point<T> dpPoint;
			dpPoint.x = pxPoint.x / (get_dpi() / 96.0f);
			dpPoint.y = pxPoint.y / (get_dpi() / 96.0f);
			return dpPoint;
		}

		template<typename T>
		Docanto::Geometry::Point<T> DpToPx(const Docanto::Geometry::Point<T>& dpPoint) const {
			Docanto::Geometry::Point<T> pxPoint;
			pxPoint.x = dpPoint.x * (get_dpi() / 96.0f);
			pxPoint.y = dpPoint.y * (get_dpi() / 96.0f);
			return pxPoint;
		}

		template<typename T>
		Docanto::Geometry::Rectangle<T> PxToDp(const Docanto::Geometry::Rectangle<T>& pxRect) const {
			Docanto::Geometry::Rectangle<T> dpRect;
			dpRect.x = pxRect.x / (get_dpi() / 96.0f);
			dpRect.y = pxRect.y / (get_dpi() / 96.0f);
			dpRect.width = pxRect.width / (get_dpi() / 96.0f);
			dpRect.height = pxRect.height / (get_dpi() / 96.0f);
			return dpRect;
		}

		template<typename T>
		Docanto::Geometry::Rectangle<T> DpToPx(const Docanto::Geometry::Rectangle<T>& dpRect) const {
			Docanto::Geometry::Rectangle<T> pxRect;
			pxRect.x = dpRect.x * (get_dpi() / 96.0f);
			pxRect.y = dpRect.y * (get_dpi() / 96.0f);
			pxRect.width = dpRect.width * (get_dpi() / 96.0f);
			pxRect.height = dpRect.height * (get_dpi() / 96.0f);
			return pxRect;
		}


		bool get_close_request() const;
		void send_close_request();
		void send_paint_request();

		void update_cursor();

		static bool is_key_pressed(VK key);


		void set_state(WINDOW_STATE state);
		// Sets a new windowsize
		void set_window_rec(Docanto::Geometry::Rectangle<long> r);
		void set_window_dim(Docanto::Geometry::Dimension<long> d);
		void set_window_pos(Docanto::Geometry::Point<long> d);

		void set_window_resizable(bool resize);
		
		void set_cursor(CURSOR_TYPE tpye);
		void set_global_cursor(CURSOR_TYPE tpye);
		void set_default_cursor();

		void set_callback_paint(std::function<void()> callback);
		void set_callback_size(std::function<void(Docanto::Geometry::Dimension<long>)> callback);
		void set_callback_moving(std::function<void(Docanto::Geometry::Point<long>)> callback);
		void set_callback_nchittest(std::function<int(Docanto::Geometry::Point<long>)> callback);
		void set_callback_key(std::function<void(VK, bool)> callback);

		void set_callback_pointer_down(std::function<void(PointerInfo)> m_callback_pointer_down	);
		void set_callback_pointer_up(std::function<void(PointerInfo)> m_callback_pointer_up);
		void set_callback_pointer_update(std::function<void(PointerInfo)> m_callback_pointer_update);
		void set_callback_pointer_wheel(std::function<void(short, bool)> m_callback_mousewheel);
		void set_callback_set_curosr(std::function<CURSOR_TYPE(Docanto::Geometry::Point<long>)> m_callback_set_cursor);

		void set_callback_pointer_down_nchittest(std::function<void(Docanto::Geometry::Point<long>, int)> callback_pointer_down_nchittest);
		void set_callback_dpi_changed(std::function<void(unsigned int)> callback_dpi_changed);

		friend class Direct2DRender;
	};
}

#endif // !_DOCANTOWIN_WINDOW_H_
