#include "UIToolbar.h"
#include "helper/AppVariables.h"
#include "pdf/PDFHandler.h"
#include <format>

DocantoWin::UIToolbar::UIToolbar(const std::wstring& UIName) : GenericUIObject(UIName, true) {
	this->set_bounds({ 100, 100 });
	this->set_pos({ 100, 50 });
}


Docanto::Geometry::Dimension<long> DocantoWin::UIToolbar::get_min_dims() {
	return Docanto::Geometry::Dimension<long>({ 50, 50 });
}

int DocantoWin::UIToolbar::hit_test(Docanto::Geometry::Point<long> where) {
	return HTCAPTION;
}

void DocantoWin::UIToolbar::pointer_press(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIToolbar::pointer_update(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIToolbar::pointer_release(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIToolbar::draw(std::shared_ptr<Direct2DRender> render) {
	float size = 18;
	float padding = 5;

	Docanto::Geometry::Point<float> offset = {};

	render->begin_draw();
	render->clear(AppVariables::Colors::get(AppVariables::Colors::TYPE::PRIMARY_COLOR));
	render->draw_text(L"Toolbar:", offset, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);
	
	
	render->end_draw();
}
