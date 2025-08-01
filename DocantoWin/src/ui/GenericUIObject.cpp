#include "GenericUIObject.h"
#include "helper/AppVariables.h"

int DocantoWin::GenericUIObject::resize_hittest(Docanto::Geometry::Point<long> p) {
	auto dims = get_bounds();
	auto resize_bound = resize_sqaure_amount * resize_square_size;
	if (Docanto::Geometry::Rectangle<float>({dims.width - resize_bound, dims.height - resize_bound ,
		resize_bound , resize_bound }).intersects(p)) {
		
		return HTBOTTOMRIGHT;
	}

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

DocantoWin::GenericUIObject::GenericUIObject(const std::wstring& UIName, bool resize) : resizable(resize) {
	m_window = std::make_shared<Window>(GetModuleHandle(NULL), UIName, resize);
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
		sys_draw();
	});

	m_window->set_callback_size([&](Docanto::Geometry::Dimension<long> d) {
		auto f_dim = (Docanto::Geometry::Dimension<float>) d;
		m_localrender->resize(d);
		m_dimension = m_window->PxToDp(f_dim);
	});

	m_window->set_callback_pointer_down([&](DocantoWin::Window::PointerInfo p) {
		m_local_mouse = p.pos;
	});

	m_window->set_callback_moving([&](Docanto::Geometry::Point<long> p) {
		auto c = ctx.lock();
		auto local_window_rect = Docanto::Geometry::Rectangle<float>(p, get_bounds());
		auto parent_window = c->window->get_window_rect();

		auto b = local_window_rect.intersects({ {parent_window.x + local_window_rect.width, parent_window.y + local_window_rect.height
			}, Docanto::Geometry::Dimension<float>({parent_window.width - local_window_rect.width * 2, parent_window.height - local_window_rect.height * 2})  });

		if (b) {
			make_float(false);
			set_pos(c->window->get_mouse_pos() - m_caption_delta_mouse);
			c->window->send_paint_request();
		}
		else {
			make_float(true);
		}
	});
}

void DocantoWin::GenericUIObject::set_context(std::weak_ptr<DocantoWin::Context> c) {
	ctx = c;
}

Docanto::Geometry::Dimension<float> DocantoWin::GenericUIObject::get_bounds() {
	return m_dimension;
}

void DocantoWin::GenericUIObject::set_bounds(Docanto::Geometry::Dimension<float> bounds) {
	if (is_floating()) {
		m_window->set_window_dim(bounds);
	}

	m_localrender->resize(m_window->DpToPx(bounds));
	m_dimension = bounds;

	make_float(!is_inbounds());
}


int DocantoWin::GenericUIObject::sys_hit_test(Docanto::Geometry::Point<long> where) {
	auto hit = resize_hittest(where);
	if (hit == HTNOWHERE) {
		return hit_test(where);
	}
	return hit;
}

bool DocantoWin::GenericUIObject::is_resizable() const {
	return resizable;
}

void DocantoWin::GenericUIObject::set_resizable(bool resize) {
	resizable = resize;
	m_window->set_window_resizable(resize);
}


bool DocantoWin::GenericUIObject::is_inbounds(Docanto::Geometry::Dimension<float> r) {
	auto obj = get_rec();
	return !(obj.x < 0 or obj.y < 0 or obj.y + obj.height > r.height or obj.x + obj.width > r.width);
}

bool DocantoWin::GenericUIObject::is_inbounds() {
	if (ctx.lock() == nullptr) {
		return true;
	}
	auto c = ctx.lock();
	auto window_rec = c->window->PxToDp(Docanto::Geometry::Dimension<float>(c->window->get_client_size()));
	return is_inbounds(window_rec);
}

Docanto::Geometry::Rectangle<float> DocantoWin::GenericUIObject::get_rec() {
	return Docanto::Geometry::Rectangle<float>(get_pos(), get_bounds());
}

void DocantoWin::GenericUIObject::set_pos(Docanto::Geometry::Point<float> where) {
	if (m_is_floating) {
		auto c = ctx.lock();
		m_window->set_window_pos({ c->window->get_window_position() + c->window->DpToPx(where) });
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
		pointer_press(m_local_mouse, hit);
		return false;
	}

	return true;
}

bool DocantoWin::GenericUIObject::sys_pointer_update(Docanto::Geometry::Point<float> where, int hit) {
	m_local_mouse = where - get_pos();

	make_float(!is_inbounds());

	switch (hit) {
	case HTCAPTION:
	{
		set_pos(where - m_caption_delta_mouse);
		break;
	}
	case HTBOTTOMRIGHT:
	{
		auto last_pos = get_pos();
		set_bounds({ m_local_mouse.x, m_local_mouse.y });
		if (is_floating()) {
			set_pos(last_pos);
		}
		break;
	}
	default:
	{
		pointer_update(m_local_mouse, hit);
		return false;
	}
	}

	return true;
}

bool DocantoWin::GenericUIObject::sys_pointer_release(Docanto::Geometry::Point<float> where, int hit) {
	m_local_mouse = where - get_pos();

	if (hit != HTCAPTION) {
		pointer_release(m_local_mouse, hit);
		return false;
	}

	return true;

}

void DocantoWin::GenericUIObject::draw_border() {
	auto render = ctx.lock()->render;
	
	render->begin_draw();
	render->draw_rect(get_rec(), AppVariables::Colors::get(AppVariables::Colors::TYPE::ACCENT_COLOR), 3);
	render->end_draw();
}

void DocantoWin::GenericUIObject::sys_draw() {
	auto render = m_localrender;
	if (!is_floating()) {
		render = ctx.lock()->render;
	}
	render->begin_draw();
	this->draw(render);

	if (is_resizable()) {
		auto a = get_rec();
		auto dims = a.dims();

		render->begin_draw();

		for (int y = 0; y < resize_sqaure_amount; ++y) {
			for (int x = 0; x < resize_sqaure_amount; ++x) {
				// Alternate filled and empty squares
				if ((x + y) % 2 == 0) {
					float left = dims.width - resize_sqaure_amount * resize_square_size + x * resize_square_size;
					float top = dims.height - resize_sqaure_amount * resize_square_size + y * resize_square_size;

					render->draw_rect_filled({ left, top, resize_square_size, resize_square_size },
						AppVariables::Colors::get(AppVariables::Colors::TYPE::SECONDARY_COLOR));
				}
			}
		}

		render->end_draw();
	
	}
	render->end_draw();
}

void DocantoWin::GenericUIObject::make_float(bool f) {
	if (m_is_floating == f) {
		return;
	}
	m_is_floating = f;
	if (f) {
		auto c = ctx.lock();
		// first put the window mouse so the dpi can be identified
		auto last_bound = get_bounds();
		m_window->set_state(Window::NORMAL);
		set_pos(c->window->get_mouse_pos());
		set_pos(c->window->get_mouse_pos() - m_caption_delta_mouse);
		m_window->set_min_dims(get_min_dims());
		set_bounds(last_bound);
	}
	else {
		m_window->set_state(Window::HIDDEN);
	}
}

bool DocantoWin::GenericUIObject::is_floating() const {
	return m_is_floating;
}
