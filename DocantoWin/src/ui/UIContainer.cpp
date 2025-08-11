#include "include.h"
#include "UIContainer.h"
#include "win/Direct2D.h"

void DocantoWin::UIContainer::draw() {
	auto c = ctx.lock();
	for (auto& ref : m_all_uiobjects) {
		if (!ref->is_floating()) {
			// add a clip 
			c->render->push_clipping_rect_to_origin(ref->get_rec());
			ref->sys_draw();
			c->render->pop_clipping_rect_to_origin();
		}
		else {
			//ref->sys_draw();
		}
	}

	if (m_hit_uiobject.first != nullptr) {
		m_hit_uiobject.first->draw_border();
	}
}

void DocantoWin::UIContainer::move_to_front(std::shared_ptr<GenericUIObject> obj) {
	auto index = std::find(m_all_uiobjects.begin(), m_all_uiobjects.end(), obj);
	if (index == m_all_uiobjects.end()) {
		return;
	}

	std::rotate(index, index + 1, m_all_uiobjects.end());
}

DocantoWin::UIContainer::UIContainer(std::weak_ptr<Context> c) {
	ctx = c;
}

bool DocantoWin::UIContainer::pointer_down(const Window::PointerInfo& p) {
	for (auto& ref : m_all_uiobjects) {
		if (!ref->get_rec().intersects(p.pos)) {
			continue;
		}

		m_hit_uiobject = { ref, ref->handle_hit_test(p.pos - ref->get_pos()) };

		m_hit_uiobject.first->handle_pointer_down(p, m_hit_uiobject.second);
		move_to_front(ref);
		is_action_active = true;
		return true;
	}
	m_hit_uiobject = {};
	return false;
}

bool DocantoWin::UIContainer::pointer_update(const Window::PointerInfo& p) {
	bool dirty = false;
	for (auto& ref : m_all_uiobjects) {
		if (!ref->get_rec().intersects(p.pos) or ref == m_hit_uiobject.first) {
			continue;
		}

		dirty = dirty or ref->handle_pointer_update(p, HTNOWHERE);
	}

	if (m_hit_uiobject.first == nullptr) {
		return dirty;
	}
	dirty = dirty or m_hit_uiobject.first->handle_pointer_update(p, m_hit_uiobject.second);
	return dirty;
}

bool DocantoWin::UIContainer::pointer_up(const Window::PointerInfo& p) {
	is_action_active = false;
	if (m_hit_uiobject.first == nullptr) {
		return false;
	}

	m_hit_uiobject.first->handle_pointer_release(p, m_hit_uiobject.second);

	return true;
}

DocantoWin::Window::CURSOR_TYPE DocantoWin::UIContainer::get_mouse(Docanto::Geometry::Point<long> p) {
	auto c = ctx.lock();
	for (auto& ref : m_all_uiobjects) {
		if (!ref->get_rec().intersects(p)) {
			continue;
		}

		return ref->handle_get_mouse(p - ref->get_pos());
	}

	if (is_action_active and m_hit_uiobject.first != nullptr) {
		return m_hit_uiobject.first->handle_get_mouse(p - m_hit_uiobject.first->get_pos());
	}

	return Window::CURSOR_TYPE::NONE;
}

void DocantoWin::UIContainer::resize(Docanto::Geometry::Dimension<long> new_dim) {
	for (auto& ref : m_all_uiobjects) {
		auto last_pos = ref->get_pos();
		ref->make_float(!ref->is_inbounds());
		ref->set_pos(last_pos);
	}
}

std::vector<std::shared_ptr<DocantoWin::GenericUIObject>>& DocantoWin::UIContainer::get_all_uiobjects_ref() {
	return m_all_uiobjects;
}

void DocantoWin::UIContainer::add(std::shared_ptr<GenericUIObject> obj) {
	m_all_uiobjects.push_back(obj);
}

std::optional<std::shared_ptr<DocantoWin::GenericUIObject>> DocantoWin::UIContainer::get_focused_object() const {
	if (m_hit_uiobject.first == nullptr) {
		return std::nullopt;
	}
	return m_hit_uiobject.first;
}
