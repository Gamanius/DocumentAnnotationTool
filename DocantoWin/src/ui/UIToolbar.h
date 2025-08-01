#pragma once

#include "GenericUIObject.h"

namespace DocantoWin {
	class UIToolbar : public GenericUIObject {
		Direct2DRender::SVGDocument pen_tool;
	public:
		UIToolbar(const std::wstring& UIName = L"Toolbar");

		Docanto::Geometry::Dimension<long> get_min_dims() override;

		int hit_test(Docanto::Geometry::Point<long> where) override;

		void pointer_press(Docanto::Geometry::Point<float> where, int hit) override;
		void pointer_update(Docanto::Geometry::Point<float> where, int hit) override;
		void pointer_release(Docanto::Geometry::Point<float> where, int hit) override;

		void draw(std::shared_ptr<Direct2DRender> render = nullptr) override;
	};
}