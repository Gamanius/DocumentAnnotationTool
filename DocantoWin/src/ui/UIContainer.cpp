#include "UIContainer.h"

void DocantoWin::UIContainer::draw() {
	for (auto& ref : m_all_uiobjects) {
		if (ref->is_inbounds(m_render->get_attached_window()->get_client_size())) {
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
		if (!ref->get_bounds().intersects(where)) {
			continue;
		}

		m_hit_uiobject = { ref, ref->hit_test(where) };

		m_hit_uiobject.first->pointer_press(where, m_hit_uiobject.second);
		break;
	}
}

void DocantoWin::UIContainer::pointer_update(Docanto::Geometry::Point<float> where) {
	if (m_hit_uiobject.first == nullptr) {
		return;
	}
	m_hit_uiobject.first->pointer_update(where, m_hit_uiobject.second);
	if (!m_hit_uiobject.first->is_inbounds(m_render->get_attached_window()->get_client_size())) {
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

	m_hit_uiobject.first->pointer_release(where, m_hit_uiobject.first->hit_test(where));

	m_hit_uiobject = {};
}

void DocantoWin::UIContainer::add(std::shared_ptr<GenericUIObject> obj) {
	m_all_uiobjects.push_back(obj);
}
