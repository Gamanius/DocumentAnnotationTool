#pragma once

#include "GenericUIObject.h"

namespace DocantoWin {
	class UIPDFManager : public GenericUIObject {
		size_t currently_hovering = ~0;
		std::vector<Docanto::Geometry::Rectangle<float>> get_save_recs();
		void update_hovering();
	public:
		UIPDFManager(std::weak_ptr<Context> c, const std::wstring& UIName = L"pdf_manager");

		Docanto::Geometry::Dimension<long> get_min_dims() override;

		int hit_test(Docanto::Geometry::Point<long> where) override;

		bool pointer_press(const Window::PointerInfo& p, int hit) override;
		bool pointer_update(const Window::PointerInfo& p, int hit) override;
		bool pointer_release(const Window::PointerInfo& p, int hit) override;

		void draw(std::shared_ptr<Direct2DRender> render = nullptr) override;

		Window::CURSOR_TYPE get_mouse(Docanto::Geometry::Point<float> where);
	};
}