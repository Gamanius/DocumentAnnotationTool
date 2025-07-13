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
		virtual float get_dpi() = 0;
	};
}


inline std::wostream& operator<<(std::wostream& os, const Docanto::Color& col) {
	return os << L"[r=" << col.r << L", g=" << col.g << L", b=" << col.b << L", a=" << col.alpha << L"]";
}

#endif // !_BASIC_RENDER_H_
