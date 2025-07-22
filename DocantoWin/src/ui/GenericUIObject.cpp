#include "GenericUIObject.h"

DocantoWin::GenericUIObject::GenericUIObject(const std::wstring& UIName) {
	m_window = std::make_shared<Window>(GetModuleHandle(NULL), UIName);
	m_localrender = std::make_shared<Direct2DRender>(m_window);

	m_window->set_callback_nchittest([&](Docanto::Geometry::Point<long> p) -> int {
		return hit_test(p);
	});

	m_window->set_callback_paint([&]() {
		draw();
	});

	m_window->set_callback_size([&](Docanto::Geometry::Dimension<long> d) {
		this->m_localrender->resize(d);
	});

}

bool DocantoWin::GenericUIObject::is_inbounds(Docanto::Geometry::Dimension<float> r) {
	auto other = get_bounds();
	
	return get_bounds().intersects({ {other.dims().width, other.dims().height},
		Docanto::Geometry::Dimension<float>({r.width - other.dims().width * 2, r.height - other.dims().height * 2}) });
}

void DocantoWin::GenericUIObject::make_float(bool f) {
	m_is_floating = f;
	if (f) {
		m_window->set_state(Window::NORMAL);
		m_window->set_min_dims(get_min_dims());
		m_window->set_window_dim(get_bounds().dims());
		POINT p;
		GetCursorPos(&p);
		m_window->set_window_pos({p.x, p.y});
	}
	else {
		m_window->set_state(Window::HIDDEN);
	}
}

bool DocantoWin::GenericUIObject::is_floating() const {
	return m_is_floating;
}
