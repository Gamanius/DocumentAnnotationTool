#include "Caption.h"

Docanto::Geometry::Rectangle<float> DocantoWin::Caption::get_close_btn_rect() const {
	auto caption_width = m_render->get_attached_window()->get_window_size().width;

	return Docanto::Geometry::Rectangle<float>(caption_width - m_caption_height, 0, m_caption_height, m_caption_height);
}

Docanto::Geometry::Rectangle<float> DocantoWin::Caption::get_max_btn_rect() const {
	auto caption_width = m_render->get_attached_window()->get_window_size().width;
	return Docanto::Geometry::Rectangle<float>(caption_width - m_caption_height * 2, 0, m_caption_height, m_caption_height);
}

Docanto::Geometry::Rectangle<float> DocantoWin::Caption::get_min_btn_rect() const {
	auto caption_width = m_render->get_attached_window()->get_window_size().width;
	return Docanto::Geometry::Rectangle<float>(caption_width - m_caption_height * 3, 0, m_caption_height, m_caption_height);
}

Docanto::Geometry::Rectangle<float> DocantoWin::Caption::get_caption_rect() const {
	auto caption_width = m_render->get_attached_window()->get_window_size().width;
	return Docanto::Geometry::Rectangle<float>({ 0, 0, (float)caption_width, m_caption_height });
}

DocantoWin::Caption::Caption(std::shared_ptr<Direct2DRender> render) : m_render(render) {
	m_caption_title_text_format = std::move(m_render->create_text_format(L"Consolas", m_render->get_attached_window()->PxToDp(m_caption_height)));
	m_title_text_color = std::move(m_render->create_brush({255, 255, 255, 255}));
	m_caption_color = std::move(m_render->create_brush({50, 50, 255, 255}));
	m_caption_button_line_color = std::move(m_render->create_brush({255, 255, 255, 255}));
}

void DocantoWin::Caption::draw() {
	const int button_thickness = 1;
	auto window = m_render->get_attached_window();

	auto dims = window->get_window_size();
	m_render->begin_draw();

	// draw the caption bar
	m_render->draw_rect_filled(get_caption_rect(), m_caption_color);

	// then draw the title
	m_render->draw_text(L"Docanto", {0, 0}, m_caption_title_text_format, m_title_text_color);
	
	auto hit = hittest(window->get_mouse_pos());

	// close button
	auto close_btn_rec = get_close_btn_rect();
	if (hit == HTCLOSE) {
		m_render->draw_rect_filled(close_btn_rec, { 255, 0, 0 });
	}
	m_render->draw_line(
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height / 4 },
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height * 3 / 4 },
		m_caption_button_line_color, button_thickness);
	m_render->draw_line(
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height / 4 },
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height * 3 / 4 },
		m_caption_button_line_color, button_thickness);

	// Maximize button
	auto max_btn_rec = get_max_btn_rect();
	m_render->draw_rect(
		Docanto::Geometry::Rectangle<float>({ max_btn_rec.x + max_btn_rec.height / 4, max_btn_rec.height / 4 },
		  { max_btn_rec.x + max_btn_rec.height * 3 / 4, max_btn_rec.height * 3 / 4}),
		m_caption_button_line_color, button_thickness);

	// Minimize
	auto min_btn_rec = get_min_btn_rect();
	m_render->draw_line(
		{ min_btn_rec.x + min_btn_rec.height / 4, min_btn_rec.height / 2 },
		{ min_btn_rec.x + min_btn_rec.height * 3 / 4, min_btn_rec.height / 2 },
		m_caption_button_line_color, button_thickness);

	m_render->end_draw();
}

int DocantoWin::Caption::hittest(Docanto::Geometry::Point<long> mousepos) const {
	auto dpi = m_render->get_attached_window()->get_dpi();
	auto caption_width = m_render->get_attached_window()->get_window_size().width;


	int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
	int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

	Docanto::Geometry::Rectangle<int> top_frame(0, 0, caption_width, frame_y + padding);
	if (top_frame.intersects(mousepos)) {
		return HTTOP;
	}

	if (get_close_btn_rect().intersects(mousepos)) {
		return HTCLOSE;
	}
	if (get_max_btn_rect().intersects(mousepos)) {
		return HTMAXBUTTON;
	}

	if (get_min_btn_rect().intersects(mousepos)) {
		return HTMINBUTTON;
	}

	if (get_caption_rect().intersects(mousepos)) {
		return HTCAPTION;
	}

	return 0;
}
