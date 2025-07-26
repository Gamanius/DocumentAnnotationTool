#include "GenericUIObject.h"

int DocantoWin::GenericUIObject::resize_hittest(Docanto::Geometry::Point<long> p) {
	auto dpi = m_window->get_dpi();

	int frame_x = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi); // Left/Right
	int frame_y = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi); // Top/Bottom

	auto r = m_window->get_client_size();


	// fix
	//if (p.y > r.height + frame_y) {
	//	return HTBOTTOM;
	//}

	return HTNOWHERE;
}

DocantoWin::GenericUIObject::GenericUIObject(const std::wstring& UIName) {
	m_window = std::make_shared<Window>(GetModuleHandle(NULL), UIName);
	m_window->override_default_hittest();
	m_localrender = std::make_shared<Direct2DRender>(m_window);

	m_window->set_callback_pointer_down_nchittest([&](Docanto::Geometry::Point<long> p, int) {
		m_caption_delta_mouse = p;
	});

	m_window->set_callback_dpi_changed([&](unsigned int dpi) {
		//m_window->set_window_dim(get_bounds());
	});

	m_window->set_callback_nchittest([&](Docanto::Geometry::Point<long> p) -> int {
		auto resize = resize_hittest(p);
		if (resize == HTNOWHERE) {
			return hit_test(p);
		}

		return resize;
	});

	m_window->set_callback_paint([&]() {
		draw();
	});

	m_window->set_callback_size([&](Docanto::Geometry::Dimension<long> d) {
		this->m_localrender->resize(d);
	});

	m_window->set_callback_pointer_down([&](DocantoWin::Window::PointerInfo p) {
		m_local_mouse = p.pos;
	});

	m_window->set_callback_moving([&](Docanto::Geometry::Point<long> p) {
		auto local_window_rect = Docanto::Geometry::Rectangle<float>(p, get_bounds());
		auto parent_window = m_parent_window->get_window_rect();

		auto b = local_window_rect.intersects({ {parent_window.x + local_window_rect.width, parent_window.y + local_window_rect.height
			}, Docanto::Geometry::Dimension<float>({parent_window.width - local_window_rect.width * 2, parent_window.height - local_window_rect.height * 2})  });

		if (b) {
			make_float(false);
			set_pos(m_parent_window->get_mouse_pos() - m_caption_delta_mouse);
			m_parent_window->send_paint_request();
		}
		else {
			make_float(true);

		}
	});
}

void DocantoWin::GenericUIObject::set_parent_window(std::shared_ptr<Window> main_window) {
	m_parent_window = main_window;
}


bool DocantoWin::GenericUIObject::is_resizable() const {
	return true;
}


bool DocantoWin::GenericUIObject::is_inbounds(Docanto::Geometry::Dimension<float> r) {
	auto obj = get_rec();
	return !(obj.x < 0 or obj.y < 0 or obj.y + obj.height > r.height or obj.x + obj.width > r.width);
}

Docanto::Geometry::Rectangle<float> DocantoWin::GenericUIObject::get_rec() {
	return Docanto::Geometry::Rectangle<float>(get_pos(), get_bounds());
}

void DocantoWin::GenericUIObject::set_pos(Docanto::Geometry::Point<float> where) {
	if (m_is_floating) {
		m_window->set_window_rec({ m_parent_window->get_window_position() + m_parent_window->DpToPx(where), get_bounds() });
	}
	m_position = where;
}

Docanto::Geometry::Point<float> DocantoWin::GenericUIObject::get_pos() {
	return m_position;
}

Docanto::Geometry::Point<float> DocantoWin::GenericUIObject::get_mouse_pos() {
	return m_local_mouse;
}


bool DocantoWin::GenericUIObject::sys_pointer_down(Docanto::Geometry::Point<float> where, int hit) {
	m_local_mouse = where - get_pos();
	m_caption_delta_mouse = m_local_mouse;

	if (hit != HTCAPTION) {
		pointer_press(where, hit);
		return false;
	}

	return true;
}

bool DocantoWin::GenericUIObject::sys_pointer_update(Docanto::Geometry::Point<float> where, int hit) {
	m_local_mouse = where - get_pos();
	auto window_rec = m_parent_window->PxToDp(Docanto::Geometry::Dimension<float>(m_parent_window->get_client_size()));

	make_float(!is_inbounds(window_rec));

	if (hit != HTCAPTION) {
		pointer_update(where, hit);
		return false;
	}

	set_pos(where - m_caption_delta_mouse);

	return true;
}

bool DocantoWin::GenericUIObject::sys_pointer_release(Docanto::Geometry::Point<float> where, int hit) {
	m_local_mouse = where - get_pos();

	if (hit != HTCAPTION) {
		pointer_release(where, hit);
		return false;
	}

	return true;

}

void DocantoWin::GenericUIObject::make_float(bool f) {
	if (m_is_floating == f) {
		return;
	}
	m_is_floating = f;
	if (f) {
		m_window->set_state(Window::NORMAL);
		// first put the window mouse so the dpi can be identified
		set_pos(m_parent_window->get_mouse_pos());
		set_pos(m_parent_window->get_mouse_pos() - m_caption_delta_mouse);
		m_window->set_min_dims(get_min_dims());
		m_window->set_window_dim(get_bounds());
	}
	else {
		m_window->set_state(Window::HIDDEN);
	}
}

bool DocantoWin::GenericUIObject::is_floating() const {
	return m_is_floating;
}
