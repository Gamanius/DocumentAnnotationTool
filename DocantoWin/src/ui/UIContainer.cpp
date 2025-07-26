#include "UIContainer.h"

void DocantoWin::UIContainer::draw() {
	auto c = ctx.lock();
	auto window = c->render->get_attached_window();
	auto window_rec = window->PxToDp(Docanto::Geometry::Dimension<float>(window->get_client_size()));
	for (auto& ref : m_all_uiobjects) {
		if (ref->is_inbounds(window_rec)) {
			ref->draw(c->render);
		}
		else {
			ref->draw();
		}
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

		m_hit_uiobject = { ref, ref->hit_test(where) };

		m_hit_uiobject.first->sys_pointer_down(where, m_hit_uiobject.second);
		break;
	}
}

void DocantoWin::UIContainer::pointer_update(Docanto::Geometry::Point<float> where) {
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

	m_hit_uiobject.first->sys_pointer_release(where, m_hit_uiobject.first->hit_test(where));

	m_hit_uiobject = {};
}


void DocantoWin::UIContainer::resize(Docanto::Geometry::Dimension<long> new_dim) {
	for (auto& ref : m_all_uiobjects) {
		auto last_pos = ref->get_pos();
		ref->make_float(!ref->is_inbounds());
		ref->set_pos(last_pos);
	}
}

void DocantoWin::UIContainer::add(std::shared_ptr<GenericUIObject> obj) {
	obj->set_context(ctx);
	m_all_uiobjects.push_back(obj);
}
