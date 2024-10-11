#pragma once

#include "WindowHandler.h"
#include "../../rsc/resource.h"
#include <dwmapi.h>
#include <windowsx.h>
#include <Vssym32.h>

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))


static WindowHandler::VK winkey_to_vk(int windowsKey) {
	switch (windowsKey) {
	case VK_LBUTTON:                           return WindowHandler::VK::LEFT_MB;
	case VK_RBUTTON:		                   return WindowHandler::VK::RIGHT_MB;
	case VK_CANCEL:			                   return WindowHandler::VK::CANCEL;
	case VK_MBUTTON:		                   return WindowHandler::VK::MIDDLE_MB;
	case VK_XBUTTON1:		                   return WindowHandler::VK::X1_MB;
	case VK_XBUTTON2:		                   return WindowHandler::VK::X2_MB;
	case VK_LSHIFT:			                   return WindowHandler::VK::LEFT_SHIFT;
	case VK_RSHIFT:			                   return WindowHandler::VK::RIGHT_SHIFT;
	case VK_LCONTROL:		                   return WindowHandler::VK::LEFT_CONTROL;
	case VK_RCONTROL:		                   return WindowHandler::VK::RIGHT_CONTROL;
	case VK_BACK:			                   return WindowHandler::VK::BACKSPACE;
	case VK_TAB:			                   return WindowHandler::VK::TAB;
	case VK_RETURN:			                   return WindowHandler::VK::ENTER;
	case VK_MENU:			                   return WindowHandler::VK::ALT;
	case VK_PAUSE:			                   return WindowHandler::VK::PAUSE;
	case VK_CAPITAL:		                   return WindowHandler::VK::CAPSLOCK;
	case VK_ESCAPE:			                   return WindowHandler::VK::ESCAPE;
	case VK_SPACE:			                   return WindowHandler::VK::SPACE;
	case VK_PRIOR:			                   return WindowHandler::VK::PAGE_UP;
	case VK_NEXT:			                   return WindowHandler::VK::PAGE_DOWN;
	case VK_END:			                   return WindowHandler::VK::END;
	case VK_HOME:			                   return WindowHandler::VK::HOME;
	case VK_LEFT:			                   return WindowHandler::VK::LEFTARROW;
	case VK_UP:				                   return WindowHandler::VK::UPARROW;
	case VK_RIGHT:			                   return WindowHandler::VK::RIGHTARROW;
	case VK_DOWN:			                   return WindowHandler::VK::DOWNARROW;
	case VK_SELECT:			                   return WindowHandler::VK::SELECT;
	case VK_PRINT:			                   return WindowHandler::VK::PRINT;
	case VK_EXECUTE:		                   return WindowHandler::VK::EXECUTE;
	case VK_SNAPSHOT:		                   return WindowHandler::VK::PRINT_SCREEN;
	case VK_INSERT:			                   return WindowHandler::VK::INSERT;
	case VK_DELETE:			                   return WindowHandler::VK::DEL;
	case VK_HELP:			                   return WindowHandler::VK::HELP;
	case 0x30:				                   return WindowHandler::VK::KEY_0;
	case 0x31:				                   return WindowHandler::VK::KEY_1;
	case 0x32:				                   return WindowHandler::VK::KEY_2;
	case 0x33:				                   return WindowHandler::VK::KEY_3;
	case 0x34:				                   return WindowHandler::VK::KEY_4;
	case 0x35:				                   return WindowHandler::VK::KEY_5;
	case 0x36:				                   return WindowHandler::VK::KEY_6;
	case 0x37:				                   return WindowHandler::VK::KEY_7;
	case 0x38:				                   return WindowHandler::VK::KEY_8;
	case 0x39:				                   return WindowHandler::VK::KEY_9;
	case 0x41:				                   return WindowHandler::VK::A;
	case 0x42:				                   return WindowHandler::VK::B;
	case 0x43:				                   return WindowHandler::VK::C;
	case 0x44:				                   return WindowHandler::VK::D;
	case 0x45:				                   return WindowHandler::VK::E;
	case 0x46:				                   return WindowHandler::VK::F;
	case 0x47:				                   return WindowHandler::VK::G;
	case 0x48:				                   return WindowHandler::VK::H;
	case 0x49:				                   return WindowHandler::VK::I;
	case 0x4a:				                   return WindowHandler::VK::J;
	case 0x4b:				                   return WindowHandler::VK::K;
	case 0x4c:				                   return WindowHandler::VK::L;
	case 0x4d:				                   return WindowHandler::VK::M;
	case 0x4e:				                   return WindowHandler::VK::N;
	case 0x4f:				                   return WindowHandler::VK::O;
	case 0x50:				                   return WindowHandler::VK::P;
	case 0x51:				                   return WindowHandler::VK::Q;
	case 0x52:				                   return WindowHandler::VK::R;
	case 0x53:				                   return WindowHandler::VK::S;
	case 0x54:				                   return WindowHandler::VK::T;
	case 0x55:				                   return WindowHandler::VK::U;
	case 0x56:				                   return WindowHandler::VK::V;
	case 0x57:				                   return WindowHandler::VK::W;
	case 0x58:				                   return WindowHandler::VK::X;
	case 0x59:				                   return WindowHandler::VK::Y;
	case 0x5a:				                   return WindowHandler::VK::Z;
	case VK_LWIN:			                   return WindowHandler::VK::LEFT_WINDOWS;
	case VK_RWIN:			                   return WindowHandler::VK::RIGHT_WINDOWS;
	case VK_APPS:			                   return WindowHandler::VK::APPLICATION;
	case VK_SLEEP:			                   return WindowHandler::VK::SLEEP;
	case VK_SCROLL:			                   return WindowHandler::VK::SCROLL_LOCK;
	case VK_LMENU:			                   return WindowHandler::VK::LEFT_MENU;
	case VK_RMENU:			                   return WindowHandler::VK::RIGHT_MENU;
	case VK_VOLUME_MUTE:	                   return WindowHandler::VK::VOLUME_MUTE;
	case VK_VOLUME_DOWN:	                   return WindowHandler::VK::VOLUME_DOWN;
	case VK_VOLUME_UP:		                   return WindowHandler::VK::VOLUME_UP;
	case VK_MEDIA_NEXT_TRACK:                  return WindowHandler::VK::MEDIA_NEXT;
	case VK_MEDIA_PREV_TRACK:                  return WindowHandler::VK::MEDIA_LAST;
	case VK_MEDIA_STOP:		                   return WindowHandler::VK::MEDIA_STOP;
	case VK_MEDIA_PLAY_PAUSE:                  return WindowHandler::VK::MEDIA_PLAY_PAUSE;
	case VK_OEM_1:			                   return WindowHandler::VK::OEM_1;
	case VK_OEM_2:			                   return WindowHandler::VK::OEM_2;
	case VK_OEM_3:			                   return WindowHandler::VK::OEM_3;
	case VK_OEM_4:			                   return WindowHandler::VK::OEM_4;
	case VK_OEM_5:			                   return WindowHandler::VK::OEM_5;
	case VK_OEM_6:			                   return WindowHandler::VK::OEM_6;
	case VK_OEM_7:			                   return WindowHandler::VK::OEM_7;
	case VK_OEM_8:			                   return WindowHandler::VK::OEM_8;
	case VK_OEM_102:                           return WindowHandler::VK::OEM_102;
	case VK_OEM_CLEAR:		                   return WindowHandler::VK::OEM_CLEAR;
	case VK_OEM_PLUS:		                   return WindowHandler::VK::OEM_PLUS;
	case VK_OEM_COMMA:		                   return WindowHandler::VK::OEM_COMMA;
	case VK_OEM_MINUS:		                   return WindowHandler::VK::OEM_MINUS;
	case VK_OEM_PERIOD:		                   return WindowHandler::VK::OEM_PERIOD;
	case VK_NUMPAD0:		                   return WindowHandler::VK::NUMPAD_0;
	case VK_NUMPAD1:		                   return WindowHandler::VK::NUMPAD_1;
	case VK_NUMPAD2:		                   return WindowHandler::VK::NUMPAD_2;
	case VK_NUMPAD3:		                   return WindowHandler::VK::NUMPAD_3;
	case VK_NUMPAD4:		                   return WindowHandler::VK::NUMPAD_4;
	case VK_NUMPAD5:		                   return WindowHandler::VK::NUMPAD_5;
	case VK_NUMPAD6:		                   return WindowHandler::VK::NUMPAD_6;
	case VK_NUMPAD7:		                   return WindowHandler::VK::NUMPAD_7;
	case VK_NUMPAD8:		                   return WindowHandler::VK::NUMPAD_8;
	case VK_NUMPAD9:		                   return WindowHandler::VK::NUMPAD_9;
	case VK_MULTIPLY:		                   return WindowHandler::VK::NUMPAD_MULTIPLY;
	case VK_ADD:			                   return WindowHandler::VK::NUMPAD_ADD;
	case VK_SEPARATOR:		                   return WindowHandler::VK::NUMPAD_SEPERATOR;
	case VK_SUBTRACT:		                   return WindowHandler::VK::NUMPAD_SUBTRACT;
	case VK_DECIMAL:		                   return WindowHandler::VK::NUMPAD_COMMA;
	case VK_DIVIDE:			                   return WindowHandler::VK::NUMPAD_DIVIDE;
	case VK_NUMLOCK:		                   return WindowHandler::VK::NUMPAD_LOCK;
	case VK_F1:				                   return WindowHandler::VK::F1;
	case VK_F2:				                   return WindowHandler::VK::F2;
	case VK_F3:				                   return WindowHandler::VK::F3;
	case VK_F4:				                   return WindowHandler::VK::F4;
	case VK_F5:				                   return WindowHandler::VK::F5;
	case VK_F6:				                   return WindowHandler::VK::F6;
	case VK_F7:				                   return WindowHandler::VK::F7;
	case VK_F8:				                   return WindowHandler::VK::F8;
	case VK_F9:				                   return WindowHandler::VK::F9;
	case VK_F10:			                   return WindowHandler::VK::F10;
	case VK_F11:			                   return WindowHandler::VK::F11;
	case VK_F12:			                   return WindowHandler::VK::F12;
	case VK_F13:			                   return WindowHandler::VK::F13;
	case VK_F14:			                   return WindowHandler::VK::F14;
	case VK_F15:			                   return WindowHandler::VK::F15;
	case VK_F16:			                   return WindowHandler::VK::F16;
	case VK_F17:			                   return WindowHandler::VK::F17;
	case VK_F18:			                   return WindowHandler::VK::F18;
	case VK_F19:			                   return WindowHandler::VK::F19;
	case VK_F20:			                   return WindowHandler::VK::F20;
	case VK_F21:			                   return WindowHandler::VK::F21;
	case VK_F22:			                   return WindowHandler::VK::F22;
	case VK_F23:			                   return WindowHandler::VK::F23;
	case VK_F24:			                   return WindowHandler::VK::F24;
	case VK_PLAY:			                   return WindowHandler::VK::PLAY;
	case VK_ZOOM: 			                   return WindowHandler::VK::ZOOM;
	default:				                   return WindowHandler::VK::UNKWON;
	}
}

static int vk_to_winkey(WindowHandler::VK key) {
	switch (key) {
	case WindowHandler::VK::LEFT_MB:		       return VK_LBUTTON;
	case WindowHandler::VK::RIGHT_MB:		       return VK_RBUTTON;
	case WindowHandler::VK::CANCEL:				   return VK_CANCEL;
	case WindowHandler::VK::MIDDLE_MB:			   return VK_MBUTTON;
	case WindowHandler::VK::X1_MB:				   return VK_XBUTTON1;
	case WindowHandler::VK::X2_MB:				   return VK_XBUTTON2;
	case WindowHandler::VK::LEFT_SHIFT:			   return VK_LSHIFT;
	case WindowHandler::VK::RIGHT_SHIFT:	       return VK_RSHIFT;
	case WindowHandler::VK::LEFT_CONTROL:	       return VK_LCONTROL;
	case WindowHandler::VK::RIGHT_CONTROL:         return VK_RCONTROL;
	case WindowHandler::VK::BACKSPACE:	           return VK_BACK;
	case WindowHandler::VK::TAB:			       return VK_TAB;
	case WindowHandler::VK::ENTER:	               return VK_RETURN;
	case WindowHandler::VK::ALT:			       return VK_MENU;
	case WindowHandler::VK::PAUSE:    	           return VK_PAUSE;
	case WindowHandler::VK::CAPSLOCK:		       return VK_CAPITAL;
	case WindowHandler::VK::ESCAPE:		           return VK_ESCAPE;
	case WindowHandler::VK::SPACE:		           return VK_SPACE;
	case WindowHandler::VK::PAGE_UP:		       return VK_PRIOR;
	case WindowHandler::VK::PAGE_DOWN:	           return VK_NEXT;
	case WindowHandler::VK::END:			       return VK_END;
	case WindowHandler::VK::HOME:			       return VK_HOME;
	case WindowHandler::VK::LEFTARROW:	           return VK_LEFT;
	case WindowHandler::VK::UPARROW:		       return VK_UP;
	case WindowHandler::VK::RIGHTARROW:	           return VK_RIGHT;
	case WindowHandler::VK::DOWNARROW:	           return VK_DOWN;
	case WindowHandler::VK::SELECT:		           return VK_SELECT;
	case WindowHandler::VK::PRINT:	               return VK_PRINT;
	case WindowHandler::VK::EXECUTE:		       return VK_EXECUTE;
	case WindowHandler::VK::PRINT_SCREEN:	       return VK_SNAPSHOT;
	case WindowHandler::VK::INSERT:		           return VK_INSERT;
	case WindowHandler::VK::DEL:			       return VK_DELETE;
	case WindowHandler::VK::HELP:			       return VK_HELP;
	case WindowHandler::VK::KEY_0:			       return 0x30;
	case WindowHandler::VK::KEY_1:			       return 0x31;
	case WindowHandler::VK::KEY_2:			       return 0x32;
	case WindowHandler::VK::KEY_3:			       return 0x33;
	case WindowHandler::VK::KEY_4:			       return 0x34;
	case WindowHandler::VK::KEY_5:			       return 0x35;
	case WindowHandler::VK::KEY_6:			       return 0x36;
	case WindowHandler::VK::KEY_7:			       return 0x37;
	case WindowHandler::VK::KEY_8:			       return 0x38;
	case WindowHandler::VK::KEY_9:			       return 0x39;
	case WindowHandler::VK::A:				       return 0x41;
	case WindowHandler::VK::B:				       return 0x42;
	case WindowHandler::VK::C:				       return 0x43;
	case WindowHandler::VK::D:				       return 0x44;
	case WindowHandler::VK::E:				       return 0x45;
	case WindowHandler::VK::F:				       return 0x46;
	case WindowHandler::VK::G:				       return 0x47;
	case WindowHandler::VK::H:				       return 0x48;
	case WindowHandler::VK::I:				       return 0x49;
	case WindowHandler::VK::J:				       return 0x4a;
	case WindowHandler::VK::K:				       return 0x4b;
	case WindowHandler::VK::L:				       return 0x4c;
	case WindowHandler::VK::M:				       return 0x4d;
	case WindowHandler::VK::N:				       return 0x4e;
	case WindowHandler::VK::O:				       return 0x4f;
	case WindowHandler::VK::P:				       return 0x50;
	case WindowHandler::VK::Q:				       return 0x51;
	case WindowHandler::VK::R:				       return 0x52;
	case WindowHandler::VK::S:				       return 0x53;
	case WindowHandler::VK::T:				       return 0x54;
	case WindowHandler::VK::U:				       return 0x55;
	case WindowHandler::VK::V:				       return 0x56;
	case WindowHandler::VK::W:				       return 0x57;
	case WindowHandler::VK::X:				       return 0x58;
	case WindowHandler::VK::Y:				       return 0x59;
	case WindowHandler::VK::Z:				       return 0x5a;
	case WindowHandler::VK::LEFT_WINDOWS:	       return VK_LWIN;
	case WindowHandler::VK::RIGHT_WINDOWS:	       return VK_RWIN;
	case WindowHandler::VK::APPLICATION:	       return VK_APPS;
	case WindowHandler::VK::SLEEP:			       return VK_SLEEP;
	case WindowHandler::VK::SCROLL_LOCK:	       return VK_SCROLL;
	case WindowHandler::VK::LEFT_MENU:		       return VK_LMENU;
	case WindowHandler::VK::RIGHT_MENU:	           return VK_RMENU;
	case WindowHandler::VK::VOLUME_MUTE:	       return VK_VOLUME_MUTE;
	case WindowHandler::VK::VOLUME_DOWN:	       return VK_VOLUME_DOWN;
	case WindowHandler::VK::VOLUME_UP:		       return VK_VOLUME_UP;
	case WindowHandler::VK::MEDIA_NEXT:	           return VK_MEDIA_NEXT_TRACK;
	case WindowHandler::VK::MEDIA_LAST:	           return VK_MEDIA_PREV_TRACK;
	case WindowHandler::VK::MEDIA_STOP:	           return VK_MEDIA_STOP;
	case WindowHandler::VK::MEDIA_PLAY_PAUSE:      return VK_MEDIA_PLAY_PAUSE;
	case WindowHandler::VK::OEM_1:				   return VK_OEM_1;
	case WindowHandler::VK::OEM_2:				   return VK_OEM_2;
	case WindowHandler::VK::OEM_3:				   return VK_OEM_3;
	case WindowHandler::VK::OEM_4:				   return VK_OEM_4;
	case WindowHandler::VK::OEM_5:				   return VK_OEM_5;
	case WindowHandler::VK::OEM_6:				   return VK_OEM_6;
	case WindowHandler::VK::OEM_7:				   return VK_OEM_7;
	case WindowHandler::VK::OEM_8:				   return VK_OEM_8;
	case WindowHandler::VK::OEM_102:               return VK_OEM_102;
	case WindowHandler::VK::OEM_CLEAR:			   return VK_OEM_CLEAR;
	case WindowHandler::VK::OEM_PLUS:		       return VK_OEM_PLUS;
	case WindowHandler::VK::OEM_COMMA:			   return VK_OEM_COMMA;
	case WindowHandler::VK::OEM_MINUS:			   return VK_OEM_MINUS;
	case WindowHandler::VK::OEM_PERIOD:	           return VK_OEM_PERIOD;
	case WindowHandler::VK::NUMPAD_0:		       return VK_NUMPAD0;
	case WindowHandler::VK::NUMPAD_1:		       return VK_NUMPAD1;
	case WindowHandler::VK::NUMPAD_2:		       return VK_NUMPAD2;
	case WindowHandler::VK::NUMPAD_3:		       return VK_NUMPAD3;
	case WindowHandler::VK::NUMPAD_4:		       return VK_NUMPAD4;
	case WindowHandler::VK::NUMPAD_5:		       return VK_NUMPAD5;
	case WindowHandler::VK::NUMPAD_6:		       return VK_NUMPAD6;
	case WindowHandler::VK::NUMPAD_7:		       return VK_NUMPAD7;
	case WindowHandler::VK::NUMPAD_8:		       return VK_NUMPAD8;
	case WindowHandler::VK::NUMPAD_9:		       return VK_NUMPAD9;
	case WindowHandler::VK::NUMPAD_MULTIPLY:       return VK_MULTIPLY;
	case WindowHandler::VK::NUMPAD_ADD:	           return VK_ADD;
	case WindowHandler::VK::NUMPAD_SEPERATOR:      return VK_SEPARATOR;
	case WindowHandler::VK::NUMPAD_SUBTRACT:       return VK_SUBTRACT;
	case WindowHandler::VK::NUMPAD_COMMA:	       return VK_DECIMAL;
	case WindowHandler::VK::NUMPAD_DIVIDE:		   return VK_DIVIDE;
	case WindowHandler::VK::NUMPAD_LOCK:	       return VK_NUMLOCK;
	case WindowHandler::VK::F1:					   return VK_F1;
	case WindowHandler::VK::F2:					   return VK_F2;
	case WindowHandler::VK::F3:					   return VK_F3;
	case WindowHandler::VK::F4:					   return VK_F4;
	case WindowHandler::VK::F5:					   return VK_F5;
	case WindowHandler::VK::F6:					   return VK_F6;
	case WindowHandler::VK::F7:					   return VK_F7;
	case WindowHandler::VK::F8:					   return VK_F8;
	case WindowHandler::VK::F9:					   return VK_F9;
	case WindowHandler::VK::F10:			       return VK_F10;
	case WindowHandler::VK::F11:			       return VK_F11;
	case WindowHandler::VK::F12:			       return VK_F12;
	case WindowHandler::VK::F13:			       return VK_F13;
	case WindowHandler::VK::F14:			       return VK_F14;
	case WindowHandler::VK::F15:			       return VK_F15;
	case WindowHandler::VK::F16:			       return VK_F16;
	case WindowHandler::VK::F17:			       return VK_F17;
	case WindowHandler::VK::F18:			       return VK_F18;
	case WindowHandler::VK::F19:			       return VK_F19;
	case WindowHandler::VK::F20:			       return VK_F20;
	case WindowHandler::VK::F21:			       return VK_F21;
	case WindowHandler::VK::F22:			       return VK_F22;
	case WindowHandler::VK::F23:			       return VK_F23;
	case WindowHandler::VK::F24:			       return VK_F24;
	case WindowHandler::VK::PLAY:			       return VK_PLAY;
	case WindowHandler::VK::ZOOM:			       return VK_ZOOM;
	default:									   return -1;
	}
}

std::unique_ptr<std::map<HWND, WindowHandler*>> WindowHandler::m_allWindowInstances;

static WindowHandler::PointerInfo parse_pointer_info(const WindowHandler& w, WPARAM wParam, LPARAM lParam) {
	WindowHandler::PointerInfo info;

	info.id = GET_POINTERID_WPARAM(wParam);
	POINTER_INPUT_TYPE pointerType = PT_POINTER;
	GetPointerType(info.id, &pointerType);

	POINT p = { 0, 0 };
	ClientToScreen(w.get_hwnd(), &p);

	tagPOINTER_INFO pointerinfo;
	switch (pointerType) {
	case PT_TOUCH:
		POINTER_TOUCH_INFO touchinfo;
		info.type = WindowHandler::POINTER_TYPE::TOUCH;
		if (!GetPointerTouchInfo(info.id, &touchinfo)) {
			return info;
		}
		pointerinfo = touchinfo.pointerInfo;
		break;
	case PT_PEN:
		POINTER_PEN_INFO peninfo;
		info.type = WindowHandler::POINTER_TYPE::STYLUS;
		if (!GetPointerPenInfo(info.id, &peninfo)) {
			return info;
		}
		info.pressure = peninfo.pressure;
		info.button1pressed = CHECK_BIT(peninfo.penFlags, 0) != 0;
		info.button2pressed = CHECK_BIT(peninfo.penFlags, 1) != 0 || CHECK_BIT(peninfo.penFlags, 2) != 0;
		pointerinfo = peninfo.pointerInfo;
		break;
	default:
		info.type = WindowHandler::POINTER_TYPE::MOUSE;
		if (!GetPointerInfo(info.id, &pointerinfo)) {
			return info;
		}
		info.button1pressed = CHECK_BIT(pointerinfo.pointerFlags, 4) != 0;
		info.button2pressed = CHECK_BIT(pointerinfo.pointerFlags, 5) != 0;
		info.button3pressed = CHECK_BIT(pointerinfo.pointerFlags, 6) != 0;
		info.button4pressed = CHECK_BIT(pointerinfo.pointerFlags, 7) != 0;
		info.button5pressed = CHECK_BIT(pointerinfo.pointerFlags, 8) != 0;
		break;
	}
	info.pos.x = pointerinfo.ptPixelLocation.x - (float)p.x;
	info.pos.y = pointerinfo.ptPixelLocation.y - (float)p.y;
	//info.pos = Point2D<float>(pointerinfo.ptPixelLocation);
	info.pos = w.PxToDp(info.pos);

	return info;
}

LRESULT WindowHandler::parse_window_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	WindowHandler* currentInstance = m_allWindowInstances->operator[](hWnd);
	// The WM_CREATE msg must be translated before anything else
	if (currentInstance == nullptr) {
		if (uMsg == WM_CREATE) {
			// add this windows to the window stack
			auto window = reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams; 
			m_allWindowInstances->operator[](hWnd) = reinterpret_cast<WindowHandler*>(window);

			// notify that the frame has changed
			SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		} 
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	LRESULT result; 
	auto dwm_has_processed = DwmDefWindowProc(hWnd, uMsg, wParam, lParam, &result);
	if (dwm_has_processed)
		return result;

	/*
	 * NON CLIENT AREA
	 */
	auto windowsize = currentInstance->get_window_size();
	auto dpi = currentInstance->get_dpi();
	float caption_size = AppVariables::WINDOWLAYOUT_TOOLBAR_HEIGHT * dpi / 96;

	Math::Rectangle<int> close_btn(windowsize.width - caption_size, 0, caption_size, caption_size);
	Math::Rectangle<int> max_btn(windowsize.width - caption_size * 2, 0, caption_size, caption_size);
	Math::Rectangle<int> min_btn(windowsize.width - caption_size * 3, 0, caption_size, caption_size);
	Math::Rectangle<int> toolbar(0, 0, windowsize.width, caption_size);

	switch (uMsg) {
	case WM_ENDSESSION:
	case WM_CLOSE:
	{
		currentInstance->m_closeRequest = true;

		// If WM_ENDSESSION is called we need to put another message in the queue
		// since the window will remain inactive and the message loop will not be called
		PostMessage(hWnd, WM_USER, 0, 0); 
		return NULL;
	}
	case WM_ACTIVATE:
	{
		currentInstance->invalidate_drawing_area();
		break;
	}
	case WM_POINTERDOWN:
	{
		if (currentInstance->m_callback_pointer_down == nullptr)
			break;
		currentInstance->m_callback_pointer_down(parse_pointer_info(*currentInstance, wParam, lParam));
		return 0;
	}
	case WM_POINTERUPDATE:
	{
		auto info = parse_pointer_info(*currentInstance, wParam, lParam); 
		if (currentInstance->m_is_mouse_tracking_window == false) {
			TRACKMOUSEEVENT e;
			e.cbSize = sizeof(TRACKMOUSEEVENT);
			e.dwFlags = TME_LEAVE;
			e.hwndTrack = hWnd;
			TrackMouseEvent(&e);
			currentInstance->m_is_mouse_tracking_window = true;
		}
		if (currentInstance->m_is_mouse_tracking_nc == false and toolbar.intersects(info.pos)) {
			TRACKMOUSEEVENT e;
			e.cbSize = sizeof(TRACKMOUSEEVENT);
			e.dwFlags = TME_NONCLIENT;
			e.hwndTrack = hWnd;
			TrackMouseEvent(&e);
			currentInstance->m_is_mouse_tracking_nc = true;
		}
		if (currentInstance->m_callback_pointer_update == nullptr)
			break;
		currentInstance->m_callback_pointer_update(info); 
		return 0;
	}
	case WM_POINTERUP:
	{
		if (currentInstance->m_callback_pointer_up == nullptr)
			break;
		currentInstance->m_callback_pointer_up(parse_pointer_info(*currentInstance, wParam, lParam));
		return 0;
	}	
	case WM_POINTERHWHEEL:
	{
		if (currentInstance->m_callback_mousewheel == nullptr)
			break;
		POINT ppp = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(currentInstance->m_hwnd, &ppp);
		auto p = Math::Point<float>(ppp);
		currentInstance->m_callback_mousewheel(-GET_WHEEL_DELTA_WPARAM(wParam), true, Math::Point<int>(currentInstance->PxToDp(p)));
		return 0;
	}
	case WM_POINTERWHEEL:
	{
		if (currentInstance->m_callback_mousewheel == nullptr)
			break;
		POINT ppp = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(currentInstance->m_hwnd, &ppp);
		auto p = Math::Point<float>(ppp);
		currentInstance->m_callback_mousewheel(GET_WHEEL_DELTA_WPARAM(wParam), GetKeyState(VK_SHIFT) & 0x8000, currentInstance->PxToDp(p));
		return 0;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		if (currentInstance->m_callback_key_down == nullptr) {
			break;
		}
		auto key = winkey_to_vk(wParam);
		currentInstance->m_callback_key_down(key);
			
		return 0;
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		if (currentInstance->m_callback_key_up == nullptr) {
			break;
		}
		auto key = winkey_to_vk(wParam);
		currentInstance->m_callback_key_up(key);

		return 0;
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

		requested_client_rect->right   -= frame_x + padding;
		requested_client_rect->left    += frame_x + padding;
		requested_client_rect->bottom  -= frame_y + padding;

		// we only need to add the padding if the window is maximized
		requested_client_rect->top     += (frame_y + padding) * currentInstance->is_window_maximized();

		return 0;
	}
	case WM_NCHITTEST:
	{
		LRESULT hit = DefWindowProc(hWnd, uMsg, wParam, lParam);
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

		auto xPos = GET_X_LPARAM(lParam);
		auto yPos = GET_Y_LPARAM(lParam);
		Math::Point<long> mousepos = { xPos, yPos };
		mousepos = mousepos - currentInstance->get_window_position();
		
		int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

		Math::Rectangle<int> top_frame(0, 0, windowsize.width, frame_y + padding);
		if (top_frame.intersects(mousepos)) {
			return HTTOP;
		}

		if (close_btn.intersects(mousepos)) {
			return HTCLOSE;
		}
		if (max_btn.intersects(mousepos)) {
			return HTMAXBUTTON;
		}

		if (min_btn.intersects(mousepos)) {
			return HTMINBUTTON;
		}
				
		if (toolbar.intersects(mousepos)) {
			return HTCAPTION;
		}
			
		break;
	}
	case WM_NCMOUSELEAVE:
		currentInstance->m_is_mouse_tracking_nc = false;
	case WM_NCMOUSEMOVE:
	{
		POINT p;
		GetCursorPos(&p); 
		Math::Point<long> mousepos = p;
		mousepos = mousepos - currentInstance->get_window_position();

		if (!currentInstance->m_callback_paint) {
			break;
		}

		if (close_btn.intersects(mousepos)) {
			currentInstance->m_callback_paint(WindowHandler::DRAW_EVENT::TOOLBAR_DRAW, reinterpret_cast<void*>(HTCLOSE));
		}
		else if (max_btn.intersects(mousepos)) {
			currentInstance->m_callback_paint(WindowHandler::DRAW_EVENT::TOOLBAR_DRAW, reinterpret_cast<void*>(HTMAXBUTTON));
		}
		else if (min_btn.intersects(mousepos)) {
			currentInstance->m_callback_paint(WindowHandler::DRAW_EVENT::TOOLBAR_DRAW, reinterpret_cast<void*>(HTMINBUTTON));
		}
		else {
			currentInstance->m_callback_paint(WindowHandler::DRAW_EVENT::TOOLBAR_DRAW, reinterpret_cast<void*>(0));
		}
		break;

	}
	// We have to handle these by ourself since windows doesn't do it?
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
			ShowWindow(currentInstance->m_hwnd, SW_MINIMIZE);
			return 0;
		case HTMAXBUTTON:
			ShowWindow(currentInstance->m_hwnd, currentInstance->is_window_maximized() ? SW_RESTORE : SW_MAXIMIZE);
			return 0;
		case HTCLOSE:
			SendMessage(currentInstance->m_hwnd, WM_CLOSE, 0, 0); 
			return 0;
		}
		break;
	}
	case WM_SIZE:
	case WM_SIZING:
	{
		if (currentInstance->m_callback_size) {
			currentInstance->m_callback_size(currentInstance->get_client_size());
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_TIMER:
	{
		if (wParam == WM_PAINT) { 
			SendMessage(hWnd, WM_PAINT, 0, 0); 
			currentInstance->m_paint_timer_active = false;
			KillTimer(hWnd, WM_PAINT);
		}
		break;
	}
	case WM_PAINT: 
	{	
		// we have to limit the amount of draw calls
		// so we dont overstress the gpu and induce 
		// high battery usage 
		if (currentInstance->m_last_paint.delta_ms() < AppVariables::APPVARIABLES_REFRESHTIME_MS) {
			if (currentInstance->m_paint_timer_active == false) {
				currentInstance->m_paint_timer_active = true;
				SetTimer(hWnd, WM_PAINT, AppVariables::APPVARIABLES_REFRESHTIME_MS, NULL); 
			}
			return true;
		}
		currentInstance->m_last_paint = Timer();
		if (currentInstance->m_callback_paint) {
			currentInstance->m_callback_paint(WindowHandler::DRAW_EVENT::NORMAL_DRAW, nullptr);  
		}
		ValidateRect(currentInstance->m_hwnd, NULL);
		return true;
	}
	case WM_DPICHANGED:
	{
		RECT* const prcNewWindow = (RECT*)lParam;
		auto newrec = Math::Rectangle(*prcNewWindow);
		Logger::log("DPI changed to ", HIWORD(wParam), ". Using new window dimensions ", newrec);
		currentInstance->set_window_size(newrec);
		return 0;
	}
	// custom messages
	case WM_PDF_BITMAP_READ:
	{
		if (currentInstance->m_callback_paint) {
			currentInstance->m_callback_paint(WindowHandler::DRAW_EVENT::PDF_BITMAP_READ, 
				reinterpret_cast<void*>(lParam));
		}
		return NULL;
	}
	case WM_PDF_BITMAP_READY:
	{
		if (currentInstance->m_callback_paint) {
			currentInstance->m_callback_paint(WindowHandler::DRAW_EVENT::PDF_BITMAP_READY, nullptr);
		}
		return NULL;
	}
	case WM_CUSTOM_MESSAGE:
	{
		if (currentInstance->m_callback_cutom_msg) {
			currentInstance->m_callback_cutom_msg((CUSTOM_WM_MESSAGE)lParam, (void*)wParam);
		}
		return NULL; 
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

HWND WindowHandler::get_hwnd() const {
	return m_hwnd;
}

HDC WindowHandler::get_hdc() const {
	return m_hdc;
}

UINT WindowHandler::get_dpi() const {
	return GetDpiForWindow(m_hwnd);
}

void WindowHandler::set_window_title(const std::wstring& s) {
	SetWindowText(m_hwnd, s.c_str()); 
}

Math::Rectangle<long> WindowHandler::get_client_size() const {
	RECT r;
	GetClientRect(m_hwnd, &r);
	return Math::Rectangle<long>(0, 0, r.right, r.bottom );
}

Math::Rectangle<long> WindowHandler::get_window_size() const { 
	RECT r;
	DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(RECT));
	return r; 
}

Math::Point<long> WindowHandler::get_window_position() const {
	auto r = get_window_size(); 
	return Math::Point<long>(r.x, r.y); 
}

bool WindowHandler::is_window_maximized() const { 
	return IsZoomed(m_hwnd) != 0;
}

UINT WindowHandler::intersect_toolbar_button(Math::Point<long> p) const {
	auto windowsize = get_window_size(); 
	auto dpi = get_dpi(); 
	float caption_size = AppVariables::WINDOWLAYOUT_TOOLBAR_HEIGHT * dpi / 96; 

	Math::Rectangle<int> close_btn(windowsize.width - caption_size, 0, caption_size, caption_size);
	Math::Rectangle<int> max_btn(windowsize.width - caption_size * 2, 0, caption_size, caption_size);
	Math::Rectangle<int> min_btn(windowsize.width - caption_size * 3, 0, caption_size, caption_size);
	Math::Rectangle<int> toolbar(0, 0, windowsize.width, caption_size);

	if (close_btn.intersects(p)) {
		return HTCLOSE;
	}
	else if (max_btn.intersects(p)) {
		return HTMAXBUTTON;
	}
	else if (min_btn.intersects(p)) {
		return HTMINBUTTON;
	}
	return 0;
}

void WindowHandler::set_window_size(Math::Rectangle<long> r) {
	SetWindowPos(m_hwnd, HWND_TOP, r.x, r.y, r.width, r.height, SWP_NOZORDER); 
}

bool WindowHandler::close_request() const {
	return m_closeRequest;
}

void WindowHandler::rest_close_request() {
	m_closeRequest = false;
}

void WindowHandler::send_close_request() {
	SendMessage(m_hwnd, WM_CLOSE, 0, 0);
}

bool WindowHandler::is_key_pressed(VK key) {
	return GetKeyState(vk_to_winkey(key)) & 0x8000;
	
}

Math::Point<long> WindowHandler::get_mouse_pos() const {
	Math::Point<long> p;
	GetCursorPos((POINT*)&p); // could be bad
	ScreenToClient(m_hwnd, (POINT*)&p);
	return PxToDp(p);
}

void WindowHandler::set_callback_paint(std::function<void(WindowHandler::DRAW_EVENT, void*)> callback) {
	m_callback_paint = callback;
}

void WindowHandler::set_callback_custom_msg(std::function<void(CUSTOM_WM_MESSAGE, void*)> callback) {
	m_callback_cutom_msg = callback;
}

void WindowHandler::set_callback_size(std::function<void(Math::Rectangle<long>)> callback) {
	m_callback_size = callback;
}

void WindowHandler::set_callback_pointer_down(std::function<void(PointerInfo)> callback) {
	m_callback_pointer_down = callback;
}

void WindowHandler::set_callback_pointer_up(std::function<void(PointerInfo)> callback) {
	m_callback_pointer_up = callback;
}

void WindowHandler::set_callback_pointer_update(std::function<void(PointerInfo)> callback) {
	m_callback_pointer_update = callback;
}

void WindowHandler::set_callback_mousewheel(std::function<void(short, bool, Math::Point<int>)> callback) {
	m_callback_mousewheel = callback;
}

void WindowHandler::set_callback_key_down(std::function<void(VK)> callback) {
	m_callback_key_down = callback;
}

void WindowHandler::set_callback_key_up(std::function<void(VK)> callback) {
	m_callback_key_up = callback;
}

void WindowHandler::invalidate_drawing_area() {
	InvalidateRect(m_hwnd, NULL, FALSE); 
}

void WindowHandler::invalidate_drawing_area(Math::Rectangle<long> rec) {
	RECT r = { rec.x, rec.y, rec.x + rec.width, rec.y + rec.height };
	InvalidateRect(m_hwnd, &r, FALSE);
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
		m_allWindowInstances = std::make_unique<std::map<HWND, WindowHandler*>>();
		ASSERT_WIN(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2), "Could not set DPI awereness"); 
	}


	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = reinterpret_cast<WNDPROC>(this->parse_window_messages); 
	wc.hInstance = instance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(3289650);
	wc.lpszClassName = windowName.c_str();
	wc.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
	ASSERT_WIN_RETURN_FALSE(RegisterClass(&wc), "Could not register Window");

	m_hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wc.lpszClassName, windowName.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, this); 
	ASSERT_WIN_RETURN_FALSE(m_hwnd, "Window creation was not succefull");
	SessionVariables::WINDOW_HANDLE = m_hwnd;  

	m_hdc = GetDC(m_hwnd);
	ASSERT_WIN_RETURN_FALSE(m_hwnd, "Could not retrieve device m_context");

	bool temp = EnableMouseInPointer(true);
	ASSERT_WIN_RETURN_FALSE(temp, "Couldn't add Mouse input into Pointer Input Stack API");

	// Does not work but i keep it
	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setuserobjectinformationw
	BOOL b = FALSE;
	auto res = SetUserObjectInformation(GetProcessWindowStation(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &b, sizeof(BOOL));
	//ASSERT_WIN_RETURN_FALSE(res != 0, "Could not set timer exception suppression"); 

	return true;
}

WindowHandler::WindowHandler(std::wstring windowname, HINSTANCE instance) {
	init(windowname, instance);
}

WindowHandler::~WindowHandler() {
	m_allWindowInstances->erase(m_hwnd);
	ASSERT_WIN(DestroyWindow(m_hwnd), "Error when destroying window");
}
