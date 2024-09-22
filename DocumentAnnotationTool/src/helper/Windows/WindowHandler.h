#pragma once

#include "../General/General.h"
#include "Macros.h"

#include <Windows.h>
#include <map>
#include <functional>

/// Custom WM_APP message to signal that the bitmap is ready to be drawn
#define WM_PDF_BITMAP_READY (WM_APP + 0x0BAD /*Magic number (rolled by fair dice)*/)
#define WM_CUSTOM_MESSAGE (WM_APP + 0x0BAD + 1)

#ifndef _WINDOW_HANDLER_H_
#define _WINDOW_HANDLER_H_

struct CachedBitmap;
class CaptionHandler;

enum CUSTOM_WM_MESSAGE {
	PDF_HANDLER_DISPLAY_LIST_UPDATE,
	PDF_HANDLER_ANNOTAION_CHANGE
};

class WindowHandler {
	HWND m_hwnd = NULL;
	HDC m_hdc = NULL;

	bool m_closeRequest = false;
	bool m_is_mouse_tracking_window = false;
	bool m_is_mouse_tracking_nc = false;

	static std::unique_ptr<std::map<HWND, WindowHandler*>> m_allWindowInstances;

	const unsigned int m_toolbar_margin = 30;

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

	enum DRAW_EVENT {
		NORMAL_DRAW,
		PDF_BITMAP_READ,
		TOOLBAR_DRAW
	};

	struct PointerInfo {
		UINT id = 0;
		POINTER_TYPE type = POINTER_TYPE::UNKNOWN;
		Math::Point<float> pos = { 0, 0 };
		UINT32 pressure = 0;
		bool button1pressed = false; /* Left mouse button or barrel button */
		bool button2pressed = false; /* Right mouse button eareser button */
		bool button3pressed = false; /* Middle mouse button */
		bool button4pressed = false; /* X1 Button */
		bool button5pressed = false; /* X2 Button */
	};

private:
	std::function<void(DRAW_EVENT, void*)> m_callback_paint;
	std::function<void(CUSTOM_WM_MESSAGE, void*)> m_callback_cutom_msg;
	std::function<void(Math::Rectangle<long>)> m_callback_size;
	std::function<void(PointerInfo)> m_callback_pointer_down;
	std::function<void(PointerInfo)> m_callback_pointer_up;
	std::function<void(PointerInfo)> m_callback_pointer_update;
	std::function<void(short, bool, Math::Point<int>)> m_callback_mousewheel;
	std::function<void(VK)> m_callback_key_down;
	std::function<void(VK)> m_callback_key_up;
public:

	static LRESULT parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Retrieves window messages for _all_ windows
	static void get_window_messages(bool blocking);

	bool init(std::wstring windowname, HINSTANCE instance);

	WindowHandler(std::wstring windowname, HINSTANCE instance);
	~WindowHandler();

	void set_state(WINDOW_STATE state);

	void set_callback_paint(std::function<void(DRAW_EVENT, void*)> callback);
	void set_callback_custom_msg(std::function<void(CUSTOM_WM_MESSAGE, void*)> callback);
	void set_callback_size(std::function<void(Math::Rectangle<long>)> callback);
	void set_callback_pointer_down(std::function<void(PointerInfo)> callback);
	void set_callback_pointer_up(std::function<void(PointerInfo)> callback);
	void set_callback_pointer_update(std::function<void(PointerInfo)> callback);
	void set_callback_mousewheel(std::function<void(short, bool, Math::Point<int>)> callback);
	void set_callback_key_down(std::function<void(VK)> callback);
	void set_callback_key_up(std::function<void(VK)> callback);
	
	void invalidate_drawing_area();
	void invalidate_drawing_area(Math::Rectangle<long> rec);

	// Returns the window handle
	HWND get_hwnd() const;

	// Returns the device context
	HDC get_hdc() const;

	// Returns the DPI of the window
	UINT get_dpi() const;

	void set_window_title(const std::wstring& s);

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

	unsigned int get_toolbar_margin() const;

	// Returns the window size
	Math::Rectangle<long> get_client_size() const;

	Math::Rectangle<long> get_window_size() const;

	Math::Point<long> get_window_position() const;

	bool is_window_maximized() const;

	UINT intersect_toolbar_button(Math::Point<long> p) const; 

	// Sets a new windowsize
	void set_window_size(Math::Rectangle<long> r);

	/// <summary>
	/// Returns true if a close request has been sent to the window
	/// </summary>
	/// <returns>true if there has been a close request</returns>
	bool close_request() const;

	static bool is_key_pressed(VK key);

	Math::Point<long> get_mouse_pos() const;


	/// <summary>
	///  returns the window handle
	/// </summary>
	operator HWND() const { return m_hwnd; }
};

#endif // !_WINDOW_HANDLER_H_