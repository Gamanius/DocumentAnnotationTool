#include "CaptionHandler.h"

CaptionHandler::CaptionHandler(Direct2DRenderer* renderer) : m_renderer(renderer) {
}

CaptionHandler::CaptionHandler(CaptionHandler&& c) noexcept {
	m_renderer = c.m_renderer;
	c.m_renderer = nullptr;
	m_caption = std::move(c.m_caption); 	
	m_caption_size = c.m_caption_size;
	c.m_caption_size = 0;
}

CaptionHandler& CaptionHandler::operator=(CaptionHandler&& c) noexcept  {
	m_renderer = c.m_renderer;
	c.m_renderer = nullptr;
	m_caption = std::move(c.m_caption);
	
	m_caption_size = c.m_caption_size;
	c.m_caption_size = 0;

	return *this;
}

void CaptionHandler::set_caption(const std::wstring& caption) {
	m_caption = caption;
}

void CaptionHandler::draw_caption() {
	m_renderer->begin_draw();
	auto scale = m_renderer->get_dpi_scale();
	m_renderer->set_identity_transform_active();
	auto win_size = m_renderer->get_window_size();

	float caption_height = m_caption_size;
	auto caption_width = win_size.width * scale;
	m_renderer->draw_rect_filled({ 0, 0, caption_width, caption_height }, { 70, 70, 70 });
	m_renderer->draw_text(APPLICATION_NAME, { 0, 0 }, { 255, 255, 255 }, caption_height);

	// close button
	m_renderer->draw_rect_filled({ caption_width - caption_height, 0, caption_height, caption_height }, { 255, 0, 0 });
	m_renderer->draw_line({ caption_width - caption_height + caption_height / 4, caption_height / 4 }, { caption_width - caption_height + caption_height - caption_height / 4, caption_height - caption_height / 4 }, { 255, 255, 255 }, 2);
	m_renderer->draw_line({ caption_width - caption_height + caption_height / 4, caption_height - caption_height / 4 }, { caption_width - caption_height + caption_height - caption_height / 4, caption_height / 4 }, { 255, 255, 255 }, 2);

	// Maximize button
	m_renderer->draw_rect_filled({ caption_width - caption_height * 2, 0, caption_height, caption_height }, { 50, 0, 0 });
	m_renderer->draw_rect({ { caption_width - caption_height * 2 + caption_height / 4, caption_height / 4 }, { caption_width - caption_height * 2 + caption_height - caption_height / 4, caption_height - caption_height / 4 } }, { 255, 255, 255 }, 2);

	// Minimize
	m_renderer->draw_rect_filled({ caption_width - caption_height * 3, 0, caption_height, caption_height }, { 255, 0, 255 });
	m_renderer->draw_line({ caption_width - caption_height * 3 + caption_height / 4, caption_height / 2 }, { caption_width - caption_height * 3 + caption_height - caption_height / 4, caption_height / 2 }, { 255, 255, 255 }, 2);

	m_renderer->end_draw();
}

LRESULT CaptionHandler::handle_hittest(Math::Point<long> mousepos, Math::Rectangle<long> windowsize) {
	int offset = 15;
	float caption_size = m_caption_size / m_renderer->get_dpi_scale();

	Math::Rectangle<int> top_left(0, 0, offset, offset);
	Math::Rectangle<int> top_right(windowsize.width - offset, 0, windowsize.width, offset);
	Math::Rectangle<int> bottom_left(0, windowsize.height - offset, offset, windowsize.height);
	Math::Rectangle<int> bottom_right(windowsize.width - offset, windowsize.height - offset, windowsize.width, windowsize.height);

	if (top_left.intersects(mousepos)) {
		return HTTOPLEFT;
	}
	if (top_right.intersects(mousepos)) {
		return HTTOPRIGHT;
	}
	if (bottom_left.intersects(mousepos)) {
		return HTBOTTOMLEFT;
	}
	if (bottom_right.intersects(mousepos)) {
		return HTBOTTOMRIGHT;
	}

	Math::Rectangle<int> top(0, 0, windowsize.width, offset); 
	Math::Rectangle<int> bottom(0, windowsize.height, windowsize.width, offset);

	Math::Rectangle<int> left(-offset, 0, offset, windowsize.height);
	Math::Rectangle<int> right(windowsize.width + offset, 0, offset, windowsize.height);

	if (top.intersects(mousepos)) {
		return HTTOP;
	}
	if (bottom.intersects(mousepos)) {
		return HTBOTTOM;
	}
	if (left.intersects(mousepos)) {
		return HTLEFT;
	}
	if (right.intersects(mousepos)) {
		return HTRIGHT;
	}

	Math::Rectangle<int> close_btn(windowsize.width - caption_size, 0, caption_size, caption_size);
	if (close_btn.intersects(mousepos)) {
		return HTCLOSE;
	}
	Math::Rectangle<int> max_btn(windowsize.width - caption_size * 2, 0, caption_size, caption_size);
	if (max_btn.intersects(mousepos)) {
		return HTMAXBUTTON;
	}

	Math::Rectangle<int> min_btn(windowsize.width - caption_size * 3, 0, caption_size, caption_size);
	if (min_btn.intersects(mousepos)) {
		return HTMINBUTTON;
	}

	Math::Rectangle<int> toolbar(0, 0, windowsize.width, caption_size);  
	if (toolbar.intersects(mousepos)) {
		return HTCAPTION;
	}


	return HTNOWHERE;
}

unsigned int CaptionHandler::get_caption_height() const {
	return m_caption_size;
}

CaptionHandler::~CaptionHandler() {
}
