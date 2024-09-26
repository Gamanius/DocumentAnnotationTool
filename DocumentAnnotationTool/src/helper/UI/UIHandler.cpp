#include "UIHandler.h"

UIHandler::UIHandler(Direct2DRenderer* renderer) {
	m_renderer = renderer;
	m_text_brush = m_renderer->create_brush(AppVariables::COLOR_TEXT);  
}

UIHandler::UIHandler(UIHandler&& other) noexcept {
	m_renderer = other.m_renderer;
	m_text_brush = std::move(other.m_text_brush);
	other.m_renderer = nullptr;
}

UIHandler& UIHandler::operator=(UIHandler&& other) noexcept {
	if (this != &other) {
		m_renderer = other.m_renderer;
		m_text_brush = std::move(other.m_text_brush);
		other.m_renderer = nullptr;
	}
	return *this;
}

void UIHandler::draw_caption(UINT btn) {
	constexpr int caption_line_width = 1;
	m_renderer->begin_draw();
	auto scale = m_renderer->get_dpi_scale();
	m_renderer->set_identity_transform_active();
	auto win_size = m_renderer->get_window_size();

	float caption_height = AppVariables::WINDOWLAYOUT_TOOLBAR_HEIGHT;
	auto caption_width = win_size.width * scale;
	m_renderer->draw_rect_filled({ 0, 0, caption_width, caption_height }, AppVariables::COLOR_PRIMARY);

	auto text = m_renderer->create_text_format(AppVariables::WINDOWLAYOUT_FONT, caption_height * 0.9);
	m_renderer->draw_text(SessionVariables::WINDOW_TITLE, { 5, 0 }, text, m_text_brush); 

	// close button
	Math::Rectangle<float> close_btn_rec(caption_width - caption_height, 0, caption_height, caption_height);
	if (btn == HTCLOSE) {
		m_renderer->draw_rect_filled(close_btn_rec, {255, 0, 0});
	}
	m_renderer->draw_line(
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height / 4 },
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height * 3 / 4 },
		{ 255, 255, 255 }, caption_line_width);
	m_renderer->draw_line(
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height / 4 },
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height * 3 / 4 },
		{ 255, 255, 255 }, caption_line_width);

	// Maximize button
	Math::Rectangle<float> max_btn_rec(caption_width - caption_height * 2, 0, caption_height, caption_height);
	if (btn == HTMAXBUTTON) {
		m_renderer->draw_rect_filled(max_btn_rec, AppVariables::COLOR_SECONDARY);
	}
	m_renderer->draw_rect(
		{ { max_btn_rec.x + max_btn_rec.height / 4, max_btn_rec.height / 4 },
		  { max_btn_rec.x + max_btn_rec.height * 3 / 4, max_btn_rec.height * 3 / 4} },
		{ 255, 255, 255 }, caption_line_width);

	// Minimize
	Math::Rectangle<float> min_btn_rec(caption_width - caption_height * 3, 0, caption_height, caption_height);
	if (btn == HTMINBUTTON) {
		m_renderer->draw_rect_filled(min_btn_rec, AppVariables::COLOR_SECONDARY);
	}
	m_renderer->draw_line(
		{ min_btn_rec.x + min_btn_rec.height / 4, min_btn_rec.height / 2 },
		{ min_btn_rec.x + min_btn_rec.height * 3 / 4, min_btn_rec.height / 2 },
		{ 255, 255, 255 }, caption_line_width);

	m_renderer->end_draw();
}

void UIHandler::draw_pen_selection(const std::vector<PenHandler::Pen>& pens, size_t selected) {
	m_renderer->begin_draw();
	m_renderer->set_identity_transform_active();
	
	auto scale = m_renderer->get_dpi_scale();
	auto win_size = m_renderer->get_window_size();

	float caption_width = pens.size() * AppVariables::PENSELECTION_PENS_WIDTH;
	float x0 = (win_size.width * scale - caption_width) / 2.0f;
	float y0 = AppVariables::PENSELECTION_Y_POSITION + AppVariables::WINDOWLAYOUT_TOOLBAR_HEIGHT;
	float dimension = AppVariables::PENSELECTION_PENS_WIDTH;

	m_renderer->draw_rect_filled({ x0, y0, caption_width, dimension}, AppVariables::COLOR_PRIMARY);
	m_renderer->draw_circle_filled({ x0, y0 + dimension / 2 }, dimension / 2, AppVariables::COLOR_PRIMARY);
	m_renderer->draw_circle_filled({ x0 + caption_width, y0 + dimension / 2 }, dimension / 2, AppVariables::COLOR_PRIMARY);

	auto text = m_renderer->create_text_format(AppVariables::WINDOWLAYOUT_FONT, dimension * 0.475);

	float offset = 0;
	for (const auto& pen : pens) {
		m_renderer->draw_rect_filled({ x0 + offset, y0, dimension, dimension }, pen.color);
		m_renderer->draw_text(std::format(L"{:.1f}", pen.width), {x0 + offset, y0 + dimension / 2}, text, m_text_brush); 
		offset += dimension;
	}

	if (selected != ~0) {
		m_renderer->draw_rect({ x0 + selected * dimension, y0, dimension, dimension }, { 255, 0, 0 }, 2);
	}
	m_renderer->end_draw();
}
