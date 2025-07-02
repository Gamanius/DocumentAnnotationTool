#include "../win/Direct2D.h"

struct Caption {
private:
	Direct2DRender::TextFormatObject m_caption_text_format;
	Direct2DRender::BrushObject m_text_color;
	Direct2DRender::BrushObject m_caption_color;

	float m_caption_height = 40.0f;

	std::shared_ptr<Direct2DRender> m_render;
public:

	Caption(std::shared_ptr<Direct2DRender> render);


	void draw();

	int hittest() const;
};