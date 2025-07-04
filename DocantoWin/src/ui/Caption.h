#include "win/Direct2D.h"

namespace DocantoWin {
	struct Caption {
	private:
		Direct2DRender::TextFormatObject m_caption_title_text_format;
		Direct2DRender::BrushObject m_title_text_color;
		Direct2DRender::BrushObject m_caption_color;
		Direct2DRender::BrushObject m_caption_button_line_color;

		float m_caption_height = 40.0f;

		std::shared_ptr<Direct2DRender> m_render;

		Docanto::Geometry::Rectangle<float> get_close_btn_rect() const;
		Docanto::Geometry::Rectangle<float> get_max_btn_rect() const;
		Docanto::Geometry::Rectangle<float> get_min_btn_rect() const;
		Docanto::Geometry::Rectangle<float> get_caption_rect() const;

	public:

		Caption(std::shared_ptr<Direct2DRender> render);

		void draw();

		int hittest(Docanto::Geometry::Point<long> mouse_pos) const;
	};
}