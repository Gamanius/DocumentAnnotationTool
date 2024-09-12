#pragma once

#include "../General/General.h"
#include "../Windows/Direct2D.h"
#include "MuPDF.h"

#ifndef _GESTUREHANDLER_H_
#define _GESTUREHANDLER_H_


class Direct2DRenderer;

class GestureHandler {
	// not owned by this class
	Direct2DRenderer* m_renderer = nullptr;
	// not owned by this class
	MuPDFHandler::PDF* m_pdf = nullptr;

	struct GestureFinger {
		UINT id = 0;
		bool active = 0;
		Math::Point<float> initial_position = { 0, 0 };
		Math::Point<float> last_position = { 0, 0 };
	};

	std::array<GestureFinger, 5> m_gesturefinger;
	Math::Point<float> m_initial_offset = { 0, 0 };

	D2D1::Matrix3x2F m_initialScaleMatrix;
	D2D1::Matrix3x2F m_initialScaleMatrixInv;

	std::optional<std::pair<size_t, Math::Point<float>>> m_selected_page = std::nullopt;

	void process_one_finger(GestureFinger& finger);
	void process_two_finger(GestureFinger& finger1, GestureFinger& finger2);

public:
	GestureHandler() {}
	GestureHandler(Direct2DRenderer* renderer, MuPDFHandler::PDF* pdf);
	GestureHandler(GestureHandler&& a) noexcept;
	GestureHandler& operator=(GestureHandler&& a) noexcept;

	// no need for a copy constructor. Only one should exist
	GestureHandler(const GestureHandler& o) = delete;
	GestureHandler& operator=(const GestureHandler& o) = delete;

	void start_gesture(const WindowHandler::PointerInfo& p);
	void update_gesture(const WindowHandler::PointerInfo& p);
	void end_gesture(const WindowHandler::PointerInfo& p);

	// scrolling with mouse
	void update_mouse(short delta, bool hwheel, Math::Point<int> center);

	void start_select_page(const WindowHandler::PointerInfo& p);
	void update_select_page(const WindowHandler::PointerInfo& p);
	void stop_select_page(const WindowHandler::PointerInfo& p);
	std::optional<size_t> get_selected_page() const;

	byte amount_finger_active() const;
	bool is_gesture_active() const;
	void check_bounds();
};

#endif // _GESTUREHANDLER_H_