#include "Gesture.h"

DocantoWin::GestureHandler::GestureHandler(std::shared_ptr<Direct2DRender> render, std::shared_ptr<PDFHandler> pdfhandler) {
	m_render = render;
	m_pdfhandler = pdfhandler;
}


void DocantoWin::GestureHandler::process_one_finger(GestureFinger& finger) {
	auto scale = m_render->get_transform_scale();
	auto offst = finger.last_position - finger.initial_position;
	Docanto::Geometry::Point<float> new_pos = (1 / scale) * offst;
	m_render->set_transform_matrix(new_pos + m_initial_offset);

}

void DocantoWin::GestureHandler::process_two_finger(GestureFinger& firstfinger, GestureFinger& secondfinger) {
	// calculate the scaling factor
	float distance = (firstfinger.last_position - secondfinger.last_position).distance();
	float initialDistance = (firstfinger.initial_position - secondfinger.initial_position).distance();
	float scale = distance / initialDistance;

	// calculate the center
	auto lastcenter = (firstfinger.last_position + secondfinger.last_position) / 2.0f;
	auto initialCenter = (firstfinger.initial_position + secondfinger.initial_position) / 2.0f;

	// set the matrix scale offset
	auto newscale = D2D1::Matrix3x2F::Scale({ (float)(scale), (float)(scale) }, m_initialScaleMatrixInv.TransformPoint(PointToD2D1(initialCenter)));
	m_render->set_scale_matrix(newscale * m_initialScaleMatrix);

	// and set the translation offset
	auto translation = (1 / m_render->get_transform_scale()) * (lastcenter - initialCenter);
	m_render->set_transform_matrix(m_initial_offset + translation);
}


void DocantoWin::GestureHandler::start_gesture(const Window::PointerInfo& p) {
	// TODO: maybe add a check if gesture is already touchpad and someone touches the screen
	m_isGestureTouchpad = p.type == Window::POINTER_TYPE::TOUCHPAD;

	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		if (m_gesturefinger.at(i).active) {
			continue;
		}

		m_gesturefinger.at(i).active = true;
		m_gesturefinger.at(i).id = p.id;
		m_gesturefinger.at(i).initial_position = p.pos;
		m_gesturefinger.at(i).last_position = p.pos;
		break;
	}

	byte amount_finger = amount_finger_active();
	switch (amount_finger) {
	case 1:
	{
		m_initial_offset = m_render->get_transform_pos();
		return;
	};
	case 2:
	{
		m_initial_offset = m_render->get_transform_pos();
		m_initialScaleMatrix = m_render->get_scale_matrix();

		m_initialScaleMatrixInv = m_initialScaleMatrix;
		m_initialScaleMatrixInv.Invert();

		for (size_t i = 0; i < m_gesturefinger.size(); i++) {
			if (!m_gesturefinger.at(i).active) {
				continue;
			}
			m_gesturefinger.at(i).initial_position = m_gesturefinger.at(i).last_position;
		}

		return;
	};
	}
}

void DocantoWin::GestureHandler::update_gesture(const Window::PointerInfo& p) {
	// check which finger gets updated
	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		if (m_gesturefinger.at(i).id == p.id) {
			m_gesturefinger.at(i).last_position = p.pos;
			break;
		}
	}

	if (m_isGestureTouchpad) {
		// if the gesture is a touchpad gesture we should only have 2 fingers. just look for the first 2
		GestureFinger firstfinger = { 0,0,{0, 0} };
		GestureFinger secondfinger = { 0,0,{0, 0} };

		for (size_t i = 0; i < m_gesturefinger.size(); i++) {
			if (!m_gesturefinger.at(i).active) {
				continue;
			}

			if (!firstfinger.active) {
				firstfinger = m_gesturefinger.at(i);
				continue;
			}
			secondfinger = m_gesturefinger.at(i);
			break;
		}

		// check the distance between those two and compare it to the initial distance
		auto init_distance = (firstfinger.initial_position - secondfinger.initial_position).distance();
		auto last_distance = (firstfinger.last_position - secondfinger.last_position).distance();

		auto last_center = (firstfinger.last_position + secondfinger.last_position) / 2.0f;
		auto init_center = (firstfinger.initial_position+ secondfinger.initial_position) / 2.0f;

		if ((init_center - last_center).distance() < AppVariables::TOUCHPAD_JITTER_DISTANCE) {
			return;
		}

		if ((init_center - last_center).distance() > AppVariables::TOUCHPAD_MIN_TRAVEL_DISTANCE and m_touchpadWasOneFingerActive) {
			m_touchpadMovedFar = true;
		}

		// if the distance is smaller than a treshold we should treat it as a one finger pan
		if ((std::abs(init_distance - last_distance) < AppVariables::TOUCHPAD_DISTANCE_ZOOM_THRESHOLD and m_touchpadWasOneFingerActive)
			or m_touchpadMovedFar) {
			GestureFinger center_finger = { 0, 0, init_center * AppVariables::TOUCHPAD_PAN_SCALE_FACTOR, last_center * AppVariables::TOUCHPAD_PAN_SCALE_FACTOR};
			
			process_one_finger(center_finger);
		}
		else {
			m_touchpadWasOneFingerActive = false;
			// we need to transform it to screen space around the mouse cursor
			// it should be considered the center
			Docanto::Geometry::Point<float> mouse_pos = m_render->get_attached_window()->get_mouse_pos();
			
			// calculate the scaling factor
			float scale = last_distance / init_distance;


			// set the matrix scale offset
			auto newscale = D2D1::Matrix3x2F::Scale({ (float)(scale), (float)(scale) }, m_initialScaleMatrixInv.TransformPoint(PointToD2D1(mouse_pos)));
			m_render->set_scale_matrix(newscale * m_initialScaleMatrix);
		}

		return;

	}

	// do one finger
	byte amount_finer = amount_finger_active();

	switch (amount_finer) {
	case 1:
	{
		for (size_t i = 0; i < m_gesturefinger.size(); i++) {
			if (!m_gesturefinger.at(i).active) {
				continue;
			}
			process_one_finger(m_gesturefinger.at(i));
			break;
		}

		//check_bounds();
		return;
	};
	case 2:
	{
		GestureFinger firstfinger = { 0,0,{0, 0} };
		GestureFinger secondfinger = { 0,0,{0, 0} };

		for (size_t i = 0; i < m_gesturefinger.size(); i++) {
			if (!m_gesturefinger.at(i).active) {
				continue;
			}

			if (!firstfinger.active) {
				firstfinger = m_gesturefinger.at(i);
				continue;
			}
			secondfinger = m_gesturefinger.at(i);
			break;
		}

		process_two_finger(firstfinger, secondfinger);

		//check_bounds();
		return;
	};
	}

}

void DocantoWin::GestureHandler::end_gesture(const Window::PointerInfo& p) {
	m_touchpadWasOneFingerActive = true;
	m_touchpadMovedFar = false;

	byte amount_finger = amount_finger_active();

	switch (amount_finger) {
	case 2:
	{
		for (size_t i = 0; i < m_gesturefinger.size(); i++) {
			if (!m_gesturefinger.at(i).active) {
				continue;
			}

			m_gesturefinger.at(i).initial_position = m_gesturefinger.at(i).last_position;
		}
	};
	}

	m_initial_offset = m_render->get_transform_pos();

	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		if (m_gesturefinger.at(i).id == p.id) {
			m_gesturefinger.at(i).active = false;
		}
	}
}


byte DocantoWin::GestureHandler::amount_finger_active() const {
	byte a = 0;

	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		a += m_gesturefinger.at(i).active;
	}
	return a;
}
