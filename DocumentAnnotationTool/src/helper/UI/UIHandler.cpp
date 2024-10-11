#include "UIHandler.h"

UIHandler::UIHandler(Direct2DRenderer* renderer) {
	m_renderer = renderer;
	m_text_brush = m_renderer->create_brush(AppVariables::COLOR_TEXT);  
}

UIHandler::UIHandler(UIHandler&& other) noexcept {
	m_renderer = other.m_renderer;
	m_penhandler = other.m_penhandler;
	m_pdf = other.m_pdf;
	m_text_brush = std::move(other.m_text_brush);
	other.m_renderer = nullptr;
}

UIHandler& UIHandler::operator=(UIHandler&& other) noexcept {
	if (this != &other) {
		m_renderer = other.m_renderer;
		m_penhandler = other.m_penhandler;
		m_pdf = other.m_pdf;
		m_text_brush = std::move(other.m_text_brush);
		other.m_renderer = nullptr;
	}
	return *this;
}

void UIHandler::add_penhandler(PenHandler* pen) {
	m_penhandler = pen;
}

void UIHandler::add_pdf(MuPDFHandler::PDF* pdf) {
	m_pdf = pdf;
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

void UIHandler::draw_new_page_button() {
	auto page_rec = m_pdf->get_pagerec();
	Math::Rectangle<float> last_page_rec; 
	{
		auto r = page_rec->get_read();
		last_page_rec = r->at(r->size() - 1); 
	}

	Math::Rectangle<float> add_new_page_box(last_page_rec.x, last_page_rec.y + last_page_rec.height + AppVariables::PDF_SEPERATION_DISTANCE
		, last_page_rec.width, 100);

	m_renderer->begin_draw();
	m_renderer->set_current_transform_active(); 
	m_renderer->draw_rect(add_new_page_box, AppVariables::COLOR_PRIMARY, 5, {2, 2});

	// Draw circle centered in the rectangle
	float radius = min(add_new_page_box.width, add_new_page_box.height) / 4;
	Math::Point<float> center(add_new_page_box.x + add_new_page_box.width / 2, add_new_page_box.y + add_new_page_box.height / 2);
	m_renderer->draw_circle(center, radius, AppVariables::COLOR_PRIMARY, 5);

	// Calculate the size of the plus symbol
	float plus_size = radius / 2; // Length of the plus arms
	float plus_thickness = 5.0f;  // Thickness of the plus arms

	// Draw the horizontal part of the plus
	Math::Rectangle<float> horizontal_plus(center.x - plus_size, center.y - plus_thickness / 2,
		plus_size * 2, plus_thickness);
	m_renderer->draw_rect_filled(horizontal_plus, AppVariables::COLOR_SECONDARY); 

	// Draw the vertical part of the plus
	Math::Rectangle<float> vertical_plus(center.x - plus_thickness / 2, center.y - plus_size,
		plus_thickness, plus_size * 2);
	m_renderer->draw_rect_filled(vertical_plus, AppVariables::COLOR_SECONDARY);


	m_renderer->end_draw();

}

void UIHandler::draw_pen_selection() {
	if (m_penhandler == nullptr) {
		return;
	}
	m_renderer->begin_draw();
	m_renderer->set_identity_transform_active();
	
	auto scale = m_renderer->get_dpi_scale();
	auto win_size = m_renderer->get_window_size();

	auto pens = m_penhandler->get_all_pens();
	float caption_width = pens.size() * AppVariables::PENSELECTION_PENS_WIDTH
		+ AppVariables::PENSELECTION_PENS_WIDTH * 3; // margin and savebuton / eraser
	float x0 = (win_size.width * scale - caption_width) / 2.0f;
	float y0 = AppVariables::PENSELECTION_Y_POSITION + AppVariables::WINDOWLAYOUT_TOOLBAR_HEIGHT;
	float dimension = AppVariables::PENSELECTION_PENS_WIDTH;

	m_renderer->draw_rect_filled({ x0, y0, caption_width, dimension}, AppVariables::COLOR_PRIMARY);
	m_renderer->draw_circle_filled({ x0, y0 + dimension / 2 }, dimension / 2, AppVariables::COLOR_PRIMARY);
	m_renderer->draw_circle_filled({ x0 + caption_width, y0 + dimension / 2 }, dimension / 2, AppVariables::COLOR_PRIMARY);

	auto text = m_renderer->create_text_format(AppVariables::WINDOWLAYOUT_FONT, dimension * 0.475);
	auto save_text = m_renderer->create_text_format(AppVariables::WINDOWLAYOUT_FONT, dimension * 0.4);

	// draw safe button
	m_renderer->draw_rect_filled({ x0, y0, dimension, dimension }, AppVariables::COLOR_SECONDARY);  
	m_renderer->draw_text(L"Save", { x0, y0 + dimension * 0.6f }, save_text, m_text_brush); 

	// draw erase button
	m_renderer->draw_rect_filled({ x0 + caption_width - dimension, y0, dimension, dimension }, AppVariables::COLOR_SECONDARY); 
	m_renderer->draw_text(L"Erase", { x0 + caption_width - dimension, y0 + dimension * 0.6f }, save_text, m_text_brush); 

	float offset = 3 * AppVariables::PENSELECTION_PENS_WIDTH / 2.0f;
	for (const auto& pen : pens) {
		m_renderer->draw_rect_filled({ x0 + offset, y0, dimension, dimension }, pen.color);
		m_renderer->draw_text(std::format(L"{:.1f}", pen.width), {x0 + offset, y0 + dimension / 2}, text, m_text_brush); 
		offset += dimension;
	}

	auto selected = SessionVariables::PENSELECTION_SELECTED_PEN;
	if (selected != ~0) {
		m_renderer->draw_rect({ x0 + selected * dimension + 3 * AppVariables::PENSELECTION_PENS_WIDTH / 2.0f , y0, dimension, dimension }, AppVariables::COLOR_TERTIARY, 2);
	}
	else {
		m_renderer->draw_rect({ x0 + caption_width - dimension, y0, dimension, dimension }, AppVariables::COLOR_TERTIARY, 2);
	}
	m_renderer->end_draw();
}

UIHandler::UI_HIT UIHandler::check_ui_hit(Math::Point<float> mousepos) {
	UI_HIT hit_event;
	hit_event.type = UI_HIT::NONE;
	if (m_penhandler == nullptr) {
		return hit_event;
	}

	auto scale = m_renderer->get_dpi_scale();
	auto win_size = m_renderer->get_window_size();
	auto pens = m_penhandler->get_all_pens(); 

	float caption_width = pens.size() * AppVariables::PENSELECTION_PENS_WIDTH
		+ AppVariables::PENSELECTION_PENS_WIDTH * 3; // margin and savebuton / eraser
	float x0 = (win_size.width * scale - caption_width) / 2.0f;
	float y0 = AppVariables::PENSELECTION_Y_POSITION + AppVariables::WINDOWLAYOUT_TOOLBAR_HEIGHT;
	float dimension = AppVariables::PENSELECTION_PENS_WIDTH;

	float offset = 3 * AppVariables::PENSELECTION_PENS_WIDTH / 2.0f;
	for (const auto& pen : pens) {
		auto rect = Math::Rectangle<float>(x0 + offset, y0, dimension, dimension);
		if (rect.intersects(mousepos)) { 
			hit_event.data = offset / dimension - 1;
			hit_event.type = UI_HIT::PEN_SELECTION;
			return hit_event;
		}
		offset += dimension;
	}

	auto rect = Math::Rectangle<float>(x0, y0, dimension, dimension); 
	if (rect.intersects(mousepos)) {
		hit_event.type = UI_HIT::SAVE_BUTTON;
		return hit_event;
	}

	rect = Math::Rectangle<float>(x0 + caption_width - dimension, y0, dimension, dimension);
	if (rect.intersects(mousepos)) {
		hit_event.type = UI_HIT::PEN_SELECTION;
		hit_event.data = -1;
		return hit_event;
	}
	
	auto page_rec = m_pdf->get_pagerec();
	Math::Rectangle<float> last_page_rec;
	{
		auto r = page_rec->get_read();
		last_page_rec = r->at(r->size() - 1);
	}

	rect = Math::Rectangle<float>(last_page_rec.x, last_page_rec.y + last_page_rec.height + AppVariables::PDF_SEPERATION_DISTANCE,
		last_page_rec.width, 100);
	if (rect.intersects(m_renderer->inv_transform_point(mousepos))) {
		hit_event.type = UI_HIT::NEW_PAGE;
		hit_event.data = -1; 
		return hit_event;
	}

	return hit_event;
}

