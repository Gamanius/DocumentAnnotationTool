#include "Gesture.h"
#include "helper/AppVariables.h"

#undef max
#undef min

DocantoWin::GestureHandler::GestureHandler(std::shared_ptr<Direct2DRender> render, std::shared_ptr<PDFHandler> pdfhandler) {
	m_render = render;
	m_pdfhandler = pdfhandler;
}


void DocantoWin::GestureHandler::process_one_finger(GestureFinger& finger) {
	// we need to get the scale offset and invert it
	auto full =  m_initialScaleMatrixInv
		* m_initialRotationMatrixInv;

	auto last_pos_local = full.TransformPoint(PointToD2D1(finger.last_position));
	auto init_pos_local = full.TransformPoint(PointToD2D1(finger.initial_position));

	m_render->set_translation_matrix(D2D1::Matrix3x2F::Translation(last_pos_local.x - init_pos_local.x, last_pos_local.y - init_pos_local.y) * m_initialTranslationMatrix);
}

auto get_signed_angle(Docanto::Geometry::Point<float> vecA, Docanto::Geometry::Point<float> vecB) {
	double dotProduct = vecA.x * vecB.x + vecA.y * vecB.y;
	double magA = sqrt(vecA.x * vecA.x + vecA.y * vecA.y);
	double magB = sqrt(vecB.x * vecB.x + vecB.y * vecB.y);
	double crossProduct = vecA.x * vecB.y - vecA.y * vecB.x;
	double cosTheta = dotProduct / (magA * magB);
	cosTheta = std::max(-1.0, std::min(1.0, cosTheta));
	return std::acos(cosTheta) * RAD_TO_DEG * (crossProduct > 0 ? 1 : -1);
}

void DocantoWin::GestureHandler::process_two_finger(GestureFinger& firstfinger, GestureFinger& secondfinger) {
	// I'm sure there must be a better way since this doesnt look right

	// Scaling
	auto full = m_initialScaleMatrixInv;

	auto first_last_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(firstfinger.last_position)));
	auto first_init_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(firstfinger.initial_position)));

	auto second_last_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(secondfinger.last_position)));
	auto second_init_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(secondfinger.initial_position)));

	auto scale =  (first_last_pos_local - second_last_pos_local).distance() / (second_init_pos_local - first_init_pos_local).distance();
	auto pivot = (first_init_pos_local + second_init_pos_local) / 2.0f;

	auto new_scale_mat = D2D1::Matrix3x2F::Scale(scale, scale, PointToD2D1(pivot));

	m_render->set_scale_matrix(new_scale_mat * m_initialScaleMatrix);

	// Rotating
	auto new_rot = D2D1::Matrix3x2F::Identity();
	if (AppVariables::TOUCH_ALLOW_ROTATION) {
		full = m_initialRotationMatrix * new_scale_mat * m_initialScaleMatrix;
		full.Invert(); 

		first_last_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(firstfinger.last_position)));
		first_init_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(firstfinger.initial_position)));

		second_last_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(secondfinger.last_position)));
		second_init_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(secondfinger.initial_position)));

		auto veca = second_init_pos_local - first_init_pos_local;
		auto vecb = second_last_pos_local - first_last_pos_local;
		pivot = (first_init_pos_local + second_init_pos_local) / 2.0f;

		auto angle = get_signed_angle(veca, vecb);
		auto new_rot = D2D1::Matrix3x2F::Rotation(angle, PointToD2D1(pivot));

		m_render->set_rotation_matrix(new_rot * m_initialRotationMatrix);
	}

	// Paning
	full = m_initialTranslationMatrix * new_rot * m_initialRotationMatrix * new_scale_mat  * m_initialScaleMatrix;
	full.Invert(); 
		
	first_last_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(firstfinger.last_position)));
	first_init_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(firstfinger.initial_position)));

	second_last_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(secondfinger.last_position)));
	second_init_pos_local = D2D1ToPoint(full.TransformPoint(PointToD2D1(secondfinger.initial_position)));

	auto init_center = (first_init_pos_local + second_init_pos_local) / 2.0f;
	auto last_center = (first_last_pos_local + second_last_pos_local) / 2.0f;

	auto new_trans = D2D1::Matrix3x2F::Translation(last_center.x - init_center.x, last_center.y - init_center.y);
	m_render->set_translation_matrix(new_trans * m_initialTranslationMatrix);
}

void DocantoWin::GestureHandler::update_local_matrices() {
	m_initial_offset = m_render->get_transform_pos();
	m_initialScaleMatrix = m_render->get_scale_matrix();
	m_initialTranslationMatrix = m_render->get_transformation_matrix();
	m_initialRotationMatrix = m_render->get_rotation_matrix();

	m_initialScaleMatrixInv = m_initialScaleMatrix;
	m_initialScaleMatrixInv.Invert();

	m_initialRotationMatrixInv = m_initialRotationMatrix;
	m_initialRotationMatrixInv.Invert();

	m_initialTransformMatrixInv = m_initialTranslationMatrix;
	m_initialTransformMatrixInv.Invert();
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

	update_local_matrices();

	byte amount_finger = amount_finger_active();
	switch (amount_finger) {
	case 1:
	{
		return;
	};
	case 2:
	{
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

	update_local_matrices();
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
