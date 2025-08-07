#pragma once

#include "GenericUIObject.h"

namespace DocantoWin {
	class Window;
	struct Window::PointerInfo;

	class UIContainer {
		std::vector<std::shared_ptr<GenericUIObject>> m_all_uiobjects;
		std::pair<std::shared_ptr<GenericUIObject>, int> m_hit_uiobject;

		std::weak_ptr<Context> ctx;

	public:
		UIContainer(std::weak_ptr<Context> c);

		void draw();
		bool pointer_down(const Window::PointerInfo& p);
		bool pointer_update(const Window::PointerInfo& p);
		bool pointer_up(const Window::PointerInfo& p);
		Window::CURSOR_TYPE get_mouse(Docanto::Geometry::Point<long> p);

		void resize(Docanto::Geometry::Dimension<long> new_dim);

		std::vector<std::shared_ptr<GenericUIObject>>& get_all_uiobjects_ref();

		void add(std::shared_ptr<GenericUIObject> obj);

		std::optional<std::shared_ptr<GenericUIObject>> get_focused_object() const;
	};
}