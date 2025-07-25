#include "UIContainer.h"

void DocantoWin::UIContainer::draw() {
	auto window = m_render->get_attached_window();
	for (auto& ref : m_all_uiobjects) {
		if (ref->is_inbounds(window->PxToDp(window->get_client_size()))) {
			ref->draw(m_render);
		}
		else {
			ref->draw();
		}
	}
}

DocantoWin::UIContainer::UIContainer(std::shared_ptr<Direct2DRender> render) {
	m_render = render;
}

void DocantoWin::UIContainer::pointer_down(Docanto::Geometry::Point<float> where) {
	for (auto& ref : m_all_uiobjects) {
		if (!ref->get_rec().intersects(where)) {
			continue;
		}

		m_hit_uiobject = { ref, ref->hit_test(where) };

		m_hit_uiobject.first->sys_pointer_down(where, m_hit_uiobject.second);
		break;
	}
}

void DocantoWin::UIContainer::pointer_update(Docanto::Geometry::Point<float> where) {
	auto window = m_render->get_attached_window();
	if (m_hit_uiobject.first == nullptr) {
		return;
	}
	m_hit_uiobject.first->sys_pointer_update(where, m_hit_uiobject.second);
	if (!m_hit_uiobject.first->is_inbounds(window->PxToDp(window->get_client_size()))) {
		m_hit_uiobject.first->make_float(true);
	}
	else {
		m_hit_uiobject.first->make_float(false);

	}
}

void DocantoWin::UIContainer::pointer_up(Docanto::Geometry::Point<float> where) {
	if (m_hit_uiobject.first == nullptr) {
		return;
	}

	m_hit_uiobject.first->sys_pointer_release(where, m_hit_uiobject.first->hit_test(where));

	m_hit_uiobject = {};
}

void DocantoWin::UIContainer::add(std::shared_ptr<GenericUIObject> obj) {
	obj->set_parent_window(m_render->get_attached_window());
	m_all_uiobjects.push_back(obj);
}
