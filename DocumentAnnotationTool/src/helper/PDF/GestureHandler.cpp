#include "GestureHandler.h"
#include "cmath"

bool GestureHandler::is_moving() const {
	if (amount_finger_active() > 1) {
		return true;
	}

	return m_moved;
}

void GestureHandler::check_bounds() {
	auto bounds     = m_pdf->get_bounds(); 
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());
	auto origin     = m_renderer->get_transform_pos() - m_renderer->inv_transform_point({ 0, 0 }); 
	// half the size of the clipspace since we want half of it filled with any pdf
	auto half_width  = clip_space.width / 2;
	auto half_height = clip_space.height / 2;

	auto offset_pos = m_renderer->get_transform_pos();

	if (std::isnan(offset_pos.x) or std::isnan(offset_pos.y)) {
		m_renderer->set_transform_matrix({ 0, 0 });
		m_renderer->set_scale_matrix(1, {0, 0});
		//Logger::warn("Page was out of bounds");
	}
	
	// check for bottom of clipspace
	auto zoom_mat = m_renderer->get_scale_matrix();
	auto scale = zoom_mat._11;
	if (clip_space.bottom() - half_height < bounds.y) {
		m_renderer->set_transform_matrix({ offset_pos.x, -zoom_mat._32/ scale + half_height });
		clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());
		offset_pos = m_renderer->get_transform_pos();
	}
	if (clip_space.y + half_height > bounds.height) {
		m_renderer->set_transform_matrix({ offset_pos.x, -zoom_mat._32/scale - bounds.height + half_height }); 
		clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());
		offset_pos = m_renderer->get_transform_pos();
	}
	if (clip_space.right() - half_width < bounds.x) { 
		m_renderer->set_transform_matrix({ -zoom_mat._31/ scale + half_width , offset_pos.y }); 
		clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized()); 
		offset_pos = m_renderer->get_transform_pos();
	}
	
	if (clip_space.x + half_width > bounds.right()) { 
		m_renderer->set_transform_matrix({ -zoom_mat._31 / scale - bounds.width + half_width, offset_pos.y });
	}
}

void GestureHandler::process_one_finger(GestureFinger& finger) {
	auto scale = m_renderer->get_transform_scale();
	auto offst = finger.last_position - finger.initial_position;
	Math::Point<float> new_pos = (1 / scale) * offst;
	m_renderer->set_transform_matrix(new_pos + m_initial_offset);

	if (offst.distance() > AppVariables::CONTROLS_TOUCH_TAP_MIN_PIXEL_DISTANCE) {
		m_moved = true;
	}
}

void GestureHandler::process_two_finger(GestureFinger& firstfinger, GestureFinger& secondfinger) {
	// calculate the scaling factor
	float distance = (firstfinger.last_position - secondfinger.last_position).distance();
	float initialDistance = (firstfinger.initial_position - secondfinger.initial_position).distance();
	float scale = distance / initialDistance;

	// calculate the center
	auto lastcenter = (firstfinger.last_position + secondfinger.last_position) / 2.0f;
	auto initialCenter = (firstfinger.initial_position + secondfinger.initial_position) / 2.0f;

	// set the matrix scale offset
	auto newscale= D2D1::Matrix3x2F::Scale({ (float)(scale), (float)(scale) }, m_initialScaleMatrixInv.TransformPoint(initialCenter)); 
	m_renderer->set_scale_matrix(newscale * m_initialScaleMatrix);  

	// and set the translation offset
	auto translation = (1 / m_renderer->get_transform_scale()) * (lastcenter - initialCenter);
	m_renderer->set_transform_matrix(m_initial_offset + translation);


}

GestureHandler::GestureHandler(Direct2DRenderer* renderer, MuPDFHandler::PDF* pdf) {
	this->m_renderer = renderer;
	this->m_pdf = pdf;
}

GestureHandler::GestureHandler(GestureHandler&& a) noexcept {
	m_renderer = a.m_renderer;
	m_pdf = a.m_pdf;

	m_gesturefinger = std::move(a.m_gesturefinger);
	m_initial_offset = a.m_initial_offset;

	m_initialScaleMatrix = a.m_initialScaleMatrix;
	m_initialScaleMatrixInv = a.m_initialScaleMatrixInv;

	m_selected_page = std::move(a.m_selected_page);
}

GestureHandler& GestureHandler::operator=(GestureHandler&& a) noexcept {
	this->~GestureHandler();
	new(this) GestureHandler(std::move(a));
	return *this;
}

void GestureHandler::start_gesture(const WindowHandler::PointerInfo& p) {
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
		m_moved = false;
		m_initial_offset = m_renderer->get_transform_pos();// - m_renderer->inv_transform_point({ 0, 0 });
		return;
	};
	case 2:
	{
		m_moved = true;
		m_initial_offset = m_renderer->get_transform_pos();
		m_initialScaleMatrix = m_renderer->get_scale_matrix();
		
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

void GestureHandler::update_gesture(const WindowHandler::PointerInfo& p) {
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

		check_bounds(); 
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

		check_bounds();
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

	m_initial_offset = m_renderer->get_transform_pos();

	for (size_t i = 0; i < m_gesturefinger.size(); i++) {
		if (m_gesturefinger.at(i).id == p.id) {
			m_gesturefinger.at(i).active = false;
		}
	}
}

void GestureHandler::update_page_pos(bool dir, bool hor, std::optional<Math::Point<int>> center) {
	if (is_gesture_active()) {
		return;
	}

	// if we get a center it means it is zoom
	auto delta = AppVariables::CONTROLS_ARROWS_OFFSET / m_renderer->get_transform_scale();
	if (center.has_value()) {
		m_renderer->add_scale_matrix(dir ? AppVariables::CONRTOLS_MOUSE_ZOOM_SCALE : 1/AppVariables::CONRTOLS_MOUSE_ZOOM_SCALE, center.value());
	} 
	else if (hor) {
		if (dir) {
			m_renderer->add_transform_matrix({ delta, 0 }); 
		}
		else {
			m_renderer->add_transform_matrix({ -delta, 0 }); 
		}
	}
	else {
		if (dir) {
			m_renderer->add_transform_matrix({ 0, delta });
		}
		else {
			m_renderer->add_transform_matrix({ 0, -delta });
		}
	}

	check_bounds();
}

void GestureHandler::start_select_page(const WindowHandler::PointerInfo& p) {
	if (m_selected_page != std::nullopt) {
		return;
	}

	auto recs = m_pdf->get_pagerec()->get_read();
	auto mouse_pos = m_renderer->inv_transform_point(p.pos);

	for (size_t i = 0; i < recs->size(); i++) {
		if (recs->at(i).intersects(mouse_pos)) {
			m_selected_page = std::make_pair(i, Math::Point<float>(p.pos));
			return;
		}
	}
}

void GestureHandler::update_select_page(const WindowHandler::PointerInfo& p) {
	if (m_selected_page == std::nullopt) {
		return;
	}

	auto recs = m_pdf->get_pagerec()->get_write(); 
	auto delta = m_renderer->inv_transform_point(p.pos) - m_renderer->inv_transform_point(m_selected_page.value().second);

	recs->at(m_selected_page.value().first).x += delta.x;
	recs->at(m_selected_page.value().first).y += delta.y;

	m_selected_page.value().second = p.pos;

	check_bounds();
}

void GestureHandler::stop_select_page(const WindowHandler::PointerInfo& p) {
	m_selected_page = std::nullopt;
}

std::optional<size_t> GestureHandler::get_selected_page() const {
	if (m_selected_page == std::nullopt) {
		return std::nullopt;
	}

	return m_selected_page.value().first;
}

byte GestureHandler::amount_finger_active() const {
	byte a = 0;

	for (size_t i = 0; i < m_gesturefinger.size(); i++) { 
		a += m_gesturefinger.at(i).active;
	}
	return a;
}

bool GestureHandler::is_gesture_active() const {
	return amount_finger_active() > 0 or m_selected_page != std::nullopt;
}
