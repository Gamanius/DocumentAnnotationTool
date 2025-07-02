#include "Caption.h"

Caption::Caption(std::shared_ptr<Direct2DRender> render) : m_render(render) {
	m_caption_text_format = std::move(m_render->create_text_format(L"Consolas", m_render->get_attached_window()->PxToDp(m_caption_height)));
	m_text_color = std::move(m_render->create_brush({255, 255, 255, 255}));
	m_caption_color = std::move(m_render->create_brush({50, 50, 255, 255}));
}

void Caption::draw() {
	auto window = m_render->get_attached_window();

	auto dims = window->get_window_size();
	m_render->begin_draw();
	m_render->draw_rect_filled({ 0, 0, (float)dims.width, m_caption_height }, m_caption_color);
	m_render->draw_text(L"Docanto", {0, 0}, m_caption_text_format, m_text_color);
	
	auto caption_width = window->get_window_size().width;

	// close button
	Docanto::Geometry::Rectangle<float> close_btn_rec(caption_width - m_caption_height, 0, m_caption_height, m_caption_height);
	close_btn_rec = close_btn_rec;
	m_render->draw_line(
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height / 4 },
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height * 3 / 4 },
		{ 255, 255, 255 }, 1);
	m_render->draw_line(
		{ close_btn_rec.x + close_btn_rec.height * 3 / 4, close_btn_rec.height / 4 },
		{ close_btn_rec.x + close_btn_rec.height / 4, close_btn_rec.height * 3 / 4 },
		{ 255, 255, 255 }, 1);

	// Maximize button
	Docanto::Geometry::Rectangle<float> max_btn_rec(caption_width - m_caption_height * 2, 0, m_caption_height, m_caption_height);
	max_btn_rec = max_btn_rec;
	m_render->draw_rect(
		Docanto::Geometry::Rectangle<float>({ max_btn_rec.x + max_btn_rec.height / 4, max_btn_rec.height / 4 },
		  { max_btn_rec.x + max_btn_rec.height * 3 / 4, max_btn_rec.height * 3 / 4}),
		{ 255, 255, 255 }, 1);

	// Minimize
	Docanto::Geometry::Rectangle<float> min_btn_rec(caption_width - m_caption_height * 3, 0, m_caption_height, m_caption_height);
	min_btn_rec = min_btn_rec;
	m_render->draw_line(
		{ min_btn_rec.x + min_btn_rec.height / 4, min_btn_rec.height / 2 },
		{ min_btn_rec.x + min_btn_rec.height * 3 / 4, min_btn_rec.height / 2 },
		{ 255, 255, 255 }, 1);

	m_render->end_draw();
}

int Caption::hittest() const {
	return 0;
}
