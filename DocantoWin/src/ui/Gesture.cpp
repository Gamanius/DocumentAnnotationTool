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
		m_initial_offset = m_render->get_transform_pos();// - m_renderer->inv_transform_point({ 0, 0 });
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
