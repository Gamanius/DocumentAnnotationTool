#include "include.h"

GestureHandler::GestureHandler(Direct2DRenderer* renderer) {
	this->m_renderer = renderer;
}

GestureHandler::GestureHandler(GestureHandler&& a) noexcept {
	m_renderer = a.m_renderer;
	m_gesturefinger = std::move(a.m_gesturefinger);
	m_initial_offset = a.m_initial_offset;
}

GestureHandler& GestureHandler::operator=(GestureHandler&& a) noexcept {
	this->~GestureHandler();
	new(this) GestureHandler(std::move(a));
	return *this;
}

void GestureHandler::start_gesture(const WindowHandler::PointerInfo& p) {
	size_t just_active = 0;
	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		if (m_gesturefinger.at(i).active) {
			continue;
		}

		just_active = i;
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
		m_initial_offset = m_renderer->get_transform_pos() - m_renderer->inv_transform_point({ 0, 0 });
		return;
	};
	case 2:
	{
		for (size_t i = 0; i < m_gesturefinger.size(); i++) {
			if (i == just_active or !m_gesturefinger.at(i).active) {
				continue;
			}

			m_renderer->set_transform_matrix(m_renderer->inv_transform_point(m_gesturefinger.at(i).last_position - m_gesturefinger.at(i).initial_position) + m_initial_offset);
			m_gesturefinger.at(i).last_position = m_gesturefinger.at(i).initial_position;
		}

		m_initial_offset = m_renderer->get_transform_pos();
		m_initialScaleMatrix = m_renderer->get_scale_matrix();
		
		m_initialScaleMatrixInv = m_initialScaleMatrix;
		m_initialScaleMatrixInv.Invert();
		return;
	};
	}
}

void GestureHandler::update_gesture(const WindowHandler::PointerInfo& p) {
	// check which finger gets updated
	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		if (m_gesturefinger.at(i).id == p.id) {
			m_gesturefinger.at(i).last_position = p.pos;
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
			auto pos = m_renderer->inv_transform_point(m_gesturefinger.at(i).last_position - m_gesturefinger.at(i).initial_position);
			m_renderer->set_transform_matrix(pos + m_initial_offset);

			m_initial_offset = m_renderer->get_transform_pos() - m_renderer->inv_transform_point({ 0, 0 });
			m_gesturefinger.at(i).initial_position = m_gesturefinger.at(i).last_position; 

		}
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

		// calculate the scaling factor
		float distance = (firstfinger.last_position- secondfinger.last_position).distance();
		float initialDistance = (firstfinger.initial_position- secondfinger.initial_position).distance();
		float scale = distance / initialDistance; 

		// calculate the center
		auto center = (firstfinger.last_position + secondfinger.last_position) / 2.0f;
		auto initialCenter = (firstfinger.initial_position+ secondfinger.initial_position) / 2.0f;

		// set the matrix scale offset
		auto mat = D2D1::Matrix3x2F::Scale({ (float)(scale), (float)(scale) }, m_initialScaleMatrixInv.TransformPoint(initialCenter));
		//m_initialScaleMatrix = mat * m_initialScaleMatrix;

		m_renderer->set_scale_matrix(mat * m_initialScaleMatrix); 
		//m_renderContext->setMatrixScaleOffset(m_initialScale * scale, initialCenter);
		// calculate the new scaled points and the translation
		auto transformedInitialCenter = m_renderer->inv_transform_point(initialCenter);  
		auto transformedCenter = m_renderer->inv_transform_point(center); 
		auto translation = (transformedCenter - transformedInitialCenter);
		m_renderer->set_transform_matrix(m_initial_offset + translation);
		
		return;

	};
	}

}

void GestureHandler::end_gesture(const WindowHandler::PointerInfo& p) {
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

	m_initial_offset = m_renderer->get_transform_pos() - m_renderer->inv_transform_point({ 0, 0 });

	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		if (m_gesturefinger.at(i).id == p.id) {
			m_gesturefinger.at(i).active = false;
		}
	}
}

byte GestureHandler::amount_finger_active() const {
	byte a = 0;

	for (size_t i = 0; i < m_gesturefinger.size(); i++) { 
		a += m_gesturefinger.at(i).active;
	}
	return a;
}

bool GestureHandler::is_gesture_active() const {
	return amount_finger_active() > 0;
}
