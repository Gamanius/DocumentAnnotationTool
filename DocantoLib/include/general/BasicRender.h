#ifndef _BASIC_RENDER_H_
#define _BASIC_RENDER_H_

#include "MathHelper.h"
#include "Common.h"

namespace Docanto {
	struct Color {
		byte r = 0, g = 0, b = 0;
		byte alpha = 255;
	};


	class BasicRender {
	public:
		virtual void draw_rect(Geometry::Rectangle<float> r, Color c) = 0;
	};
}

#endif // !_BASIC_RENDER_H_
