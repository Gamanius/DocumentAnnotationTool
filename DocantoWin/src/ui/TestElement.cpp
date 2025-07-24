#include "TestElement.h"

DocantoWin::TestElement::TestElement(const std::wstring& UIName) : GenericUIObject(UIName) {
	set_pos({ 100, 100 });
}


Docanto::Geometry::Dimension<float> DocantoWin::TestElement::get_bounds() {
	return { 50.0f, 50.0f };
}

Docanto::Geometry::Dimension<long> DocantoWin::TestElement::get_min_dims() {
	return get_bounds();
}

int DocantoWin::TestElement::hit_test(Docanto::Geometry::Point<long> where) {
	int frame_y = GetSystemMetricsForDpi(SM_CYSIZEFRAME, m_window->get_dpi());
	if (where.y < frame_y) {
		return HTTOP;
	}
	return HTCAPTION;
}

void DocantoWin::TestElement::pointer_press(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::TestElement::pointer_update(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::TestElement::pointer_release(Docanto::Geometry::Point<float> where, int hit) {
}


void DocantoWin::TestElement::draw(std::shared_ptr<Direct2DRender> render) {
	if (render) {
		render->draw_rect_filled({get_pos(), get_bounds()}, {255});
	}
	else {
		this->m_localrender->begin_draw();
		this->m_localrender->set_identity_transform_active();
		this->m_localrender->clear({100, 100 ,100});
		this->m_localrender->draw_rect_filled({ {0, 0,}, get_bounds()}, {255});
		this->m_localrender->end_draw();
	}
}
