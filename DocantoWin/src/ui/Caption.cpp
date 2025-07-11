#include "Caption.h"

#include "helper/AppVariables.h"

std::tuple<Docanto::Geometry::Rectangle<float>, Docanto::Geometry::Rectangle<float>, Docanto::Geometry::Rectangle<float>, Docanto::Geometry::Rectangle<float>> DocantoWin::Caption::get_caption_rects() const {
	auto window = m_render->get_attached_window();

	auto caption_width = window->PxToDp(window->get_window_size().width);
	auto is_maximized = window->is_window_maximized();

	float padding = static_cast<float>(GetSystemMetricsForDpi(SM_CYSIZEFRAME, window->get_dpi()) * is_maximized);
	padding = window->PxToDp(padding);
  
	return {
		{ 0, padding, (float)caption_width, m_caption_height },								// Caption bar
		{ caption_width - m_caption_height, padding, m_caption_height, m_caption_height },		// close btn
		{ caption_width - m_caption_height * 2, padding, m_caption_height, m_caption_height },	// maxi  btn
		{ caption_width - m_caption_height * 3, padding, m_caption_height, m_caption_height },	// mini  btn

	};
}

void DocantoWin::Caption::update_colors() {
	m_caption_color->SetColor(ColorToD2D1(AppVariables::Colors::get(AppVariables::Colors::TYPE::PRIMARY_COLOR)));
	m_caption_button_line_color->SetColor(ColorToD2D1(AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR)));
	m_title_text_color->SetColor(ColorToD2D1(AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR)));
	m_caption_button_rect_color->SetColor(ColorToD2D1(AppVariables::Colors::get(AppVariables::Colors::TYPE::SECONDARY_COLOR)));
}

DocantoWin::Caption::Caption(std::shared_ptr<Direct2DRender> render) : m_render(render) {
	m_caption_title_text_format = std::move(m_render->create_text_format(L"Consolas", m_caption_height));
	m_title_text_color = std::move(m_render->create_brush({255, 255, 255, 255}));
	m_caption_color = std::move(m_render->create_brush({50, 50, 255, 255}));
	m_caption_button_line_color = std::move(m_render->create_brush({255, 255, 255, 255}));
	m_caption_button_rect_color = std::move(m_render->create_brush({255, 255, 255, 255}));

	update_colors();
}

void DocantoWin::Caption::draw() {
	update_colors();
	const int button_thickness = 1;
	auto window = m_render->get_attached_window();

	auto dims = window->get_window_size();
	m_render->begin_draw();


	auto [
		caption_rec,
		close_btn_rec,
		max_btn_rec,
		min_btn_rec
	] = get_caption_rects();

	// draw the caption bar
	m_render->draw_rect_filled(caption_rec, m_caption_color);

	// then draw the title
	m_render->draw_text(L"Docanto", {0, 0}, m_caption_title_text_format, m_title_text_color);
	
	auto hit = hittest(window->get_mouse_pos());

	// close button
	if (hit == HTCLOSE) {
		m_render->draw_rect_filled(close_btn_rec, {255, 0, 0});
	}
	m_render->draw_line(
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height / 4 + close_btn_rec.y },
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height * 3 / 4 + close_btn_rec.y},
		m_caption_button_line_color, button_thickness);
	m_render->draw_line(
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height / 4 + close_btn_rec.y },
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height * 3 / 4 + close_btn_rec.y },
		m_caption_button_line_color, button_thickness);

	// Maximize button
	if (hit == HTMAXBUTTON) {
		m_render->draw_rect_filled(max_btn_rec, m_caption_button_rect_color);
	}
	m_render->draw_rect(
		Docanto::Geometry::Rectangle<float>({ std::ceil(max_btn_rec.x + max_btn_rec.height / 4) + 0.5f, std::ceil(max_btn_rec.height / 4 + max_btn_rec.y) + 0.5f},
		  { std::ceil(max_btn_rec.x + max_btn_rec.height * 3 / 4) + 0.5f, std::ceil(max_btn_rec.height * 3 / 4 + max_btn_rec.y) + 0.5f}),
		m_caption_button_line_color, button_thickness);

	// Minimize
	if (hit == HTMINBUTTON) {
		m_render->draw_rect_filled(min_btn_rec, m_caption_button_rect_color);
	}
	m_render->draw_line(
		{ std::ceil(min_btn_rec.x + min_btn_rec.height / 4) + 0.5f, std::ceil(min_btn_rec.height / 2 + min_btn_rec.y) + 0.5f },
		{ std::ceil(min_btn_rec.x + min_btn_rec.height * 3 / 4) + 0.5f, std::ceil(min_btn_rec.height / 2 + min_btn_rec.y) + 0.5f},
		m_caption_button_line_color, button_thickness);

	m_render->end_draw();
}

int DocantoWin::Caption::hittest(Docanto::Geometry::Point<long> mousepos) const {
	auto window = m_render->get_attached_window();
	auto dpi = window->get_dpi();
	auto caption_width = window->get_window_size().width;

	int frame_y = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi);

	auto [
		caption_rec,
		close_btn_rec,
		max_btn_rec,
		min_btn_rec
	] = get_caption_rects();

	Docanto::Geometry::Rectangle<int> top_frame(0, 0, caption_width, frame_y);
	if (!window->is_window_maximized() and top_frame.intersects(mousepos)) {
		return HTTOP;
	}

	if (close_btn_rec.intersects(mousepos)) {
		return HTCLOSE;
	}
	if (max_btn_rec.intersects(mousepos)) {
		return HTMAXBUTTON;
	}

	if (min_btn_rec.intersects(mousepos)) {
		return HTMINBUTTON;
	}

	if (caption_rec.intersects(mousepos)) {
		return HTCAPTION;
	}

	return 0;
}
