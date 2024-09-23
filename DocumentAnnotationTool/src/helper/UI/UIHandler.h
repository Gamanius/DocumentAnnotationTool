#pragma once
#include "../Windows/Direct2D.h"
#include "../PDF/PenHandler.h"

#ifndef _UI_HANDLER_H_ 
#define _UI_HANDLER_H_

class UIHandler {
	// not owned by this class
	Direct2DRenderer* m_renderer = nullptr; 

	Direct2DRenderer::BrushObject m_text_brush;

public:
	UIHandler(Direct2DRenderer* renderer);
	UIHandler(UIHandler&& other) noexcept;
	UIHandler& operator=(UIHandler&& other) noexcept;
	UIHandler(const UIHandler& other) = delete;
	UIHandler& operator=(const UIHandler& other) = delete;


	void draw_caption(std::wstring title, size_t height = 30, UINT btn = 0);
	void draw_pen_selection(const std::vector<PenHandler::Pen>& pens, float height, size_t selected = ~0, float dimension = 30);
};

#endif