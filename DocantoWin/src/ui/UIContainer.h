#pragma once

#include "GenericUIObject.h"

namespace DocantoWin {
	class Window;
	struct Window::PointerInfo;

	class UIContainer {
		std::vector<std::shared_ptr<GenericUIObject>> m_all_uiobjects;
		std::pair<std::shared_ptr<GenericUIObject>, int> m_hit_uiobject;
		std::shared_ptr<GenericUIObject> m_hover_target = nullptr;

		std::weak_ptr<Context> ctx;

		bool is_action_active = false;

		void move_to_front(std::shared_ptr<GenericUIObject>);

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