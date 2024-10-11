#pragma once
#include "../Windows/Direct2D.h"
#include "../PDF/PenHandler.h"

#ifndef _UI_HANDLER_H_ 
#define _UI_HANDLER_H_

class UIHandler {
	// not owned by this class
	Direct2DRenderer* m_renderer = nullptr; 
	// not owned by this class
	PenHandler* m_penhandler = nullptr;
	// not owned by this class
	MuPDFHandler::PDF* m_pdf = nullptr;

	Direct2DRenderer::BrushObject m_text_brush;


public:
	struct UI_HIT {
		enum EVENT {
			NONE = 0,
			PEN_SELECTION,
			SAVE_BUTTON,
			NEW_PAGE
		};

		EVENT type = NONE;
		int data = -1;
	};

	UIHandler(Direct2DRenderer* renderer);
	UIHandler(UIHandler&& other) noexcept;
	UIHandler& operator=(UIHandler&& other) noexcept;
	UIHandler(const UIHandler& other) = delete;
	UIHandler& operator=(const UIHandler& other) = delete;

	void add_penhandler(PenHandler* pen);
	void add_pdf(MuPDFHandler::PDF* pdf);

	/// <summary>
	/// Draws the caption bar on the top
	/// The title, height and color are defined in appvariables
	/// </summary>
	/// <param name="btn"></param>
	void draw_caption(UINT btn = 0);
	void draw_new_page_button(); 
	void draw_pen_selection();

	UI_HIT check_ui_hit(Math::Point<float> mousepos);
};

#endif