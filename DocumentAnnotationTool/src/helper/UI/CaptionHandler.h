#pragma once

#include "../Windows/Direct2D.h"

class CaptionHandler {
	Direct2DRenderer* m_renderer = nullptr;
	std::wstring m_caption;

	unsigned int m_caption_size = APPLICATION_TOOLBAR_HEIGHT;
public:
	CaptionHandler() = default;
	CaptionHandler(Direct2DRenderer* renderer);
	CaptionHandler(CaptionHandler&& c) noexcept;
	CaptionHandler& operator=(CaptionHandler&& c) noexcept;
	
	void set_caption(const std::wstring& caption);
	void draw_caption();
	LRESULT handle_hittest(Math::Point<long> mousepos, Math::Rectangle<long> window_size);
	unsigned int get_caption_height() const;
	~CaptionHandler();
};

