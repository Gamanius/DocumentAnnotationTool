#include "UIContainer.h"

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

DocantoWin::UIContainer::UIContainer(std::weak_ptr<Context> c) {
	ctx = c;
}

bool DocantoWin::UIContainer::pointer_down(const Window::PointerInfo& p) {
	for (auto& ref : m_all_uiobjects) {
		if (!ref->get_rec().intersects(p.pos)) {
			continue;
		}

		m_hit_uiobject = { ref, ref->sys_hit_test(p.pos - ref->get_pos()) };

		m_hit_uiobject.first->sys_pointer_down(p, m_hit_uiobject.second);
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

		dirty = dirty or ref->sys_pointer_update(p, HTNOWHERE);
	}

	if (m_hit_uiobject.first == nullptr) {
		return dirty;
	}
	dirty = dirty or m_hit_uiobject.first->sys_pointer_update(p, m_hit_uiobject.second);
	return dirty;
}

bool DocantoWin::UIContainer::pointer_up(const Window::PointerInfo& p) {
	if (m_hit_uiobject.first == nullptr) {
		return false;
	}

	m_hit_uiobject.first->sys_pointer_release(p, m_hit_uiobject.second);

	return true;
}

DocantoWin::Window::CURSOR_TYPE DocantoWin::UIContainer::get_mouse(Docanto::Geometry::Point<long> p) {
	auto c = ctx.lock();
	for (auto& ref : m_all_uiobjects) {
		if (!ref->get_rec().intersects(p)) {
			continue;
		}

		return ref->do_get_mouse(p);
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
