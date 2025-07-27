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
			ref->sys_draw();
		}
	}

	if (m_hit_uiobject.first != nullptr) {
		m_hit_uiobject.first->draw_border();
	}
}

DocantoWin::UIContainer::UIContainer(std::weak_ptr<Context> c) {
	ctx = c;
}

void DocantoWin::UIContainer::pointer_down(Docanto::Geometry::Point<float> where) {
	for (auto& ref : m_all_uiobjects) {
		if (!ref->get_rec().intersects(where)) {
			continue;
		}

		m_hit_uiobject = { ref, ref->sys_hit_test(where - ref->get_pos()) };

		m_hit_uiobject.first->sys_pointer_down(where, m_hit_uiobject.second);
		break;
	}
}

void DocantoWin::UIContainer::pointer_update(Docanto::Geometry::Point<float> where) {
	int hit = HTNOWHERE;
	for (auto& ref : m_all_uiobjects) {
		hit = ref->sys_hit_test(where - ref->get_pos());
		if (hit == HTBOTTOMRIGHT) {
			ctx.lock()->window->set_cursor(Window::CURSOR_TYPE::NWSE_RESIZE);
			break;
		}
	}
	if (hit != HTBOTTOMRIGHT) {
		ctx.lock()->window->set_cursor(Window::CURSOR_TYPE::POINTER);
	}

	auto window = ctx.lock()->render->get_attached_window();
	if (m_hit_uiobject.first == nullptr) {
		return;
	}
	m_hit_uiobject.first->sys_pointer_update(where, m_hit_uiobject.second);
}

void DocantoWin::UIContainer::pointer_up(Docanto::Geometry::Point<float> where) {
	if (m_hit_uiobject.first == nullptr) {
		return;
	}

	m_hit_uiobject.first->sys_pointer_release(where, m_hit_uiobject.second);

	m_hit_uiobject = {};
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
	obj->set_context(ctx);
	m_all_uiobjects.push_back(obj);
}
