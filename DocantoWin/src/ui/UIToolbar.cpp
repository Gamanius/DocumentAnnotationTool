#include "UIToolbar.h"
#include "helper/AppVariables.h"
#include "pdf/PDFHandler.h"
#include <format>

#include "../../rsc/resource.h"

DocantoWin::UIToolbar::UIToolbar(const std::wstring& UIName) : GenericUIObject(UIName, true) {
	this->set_bounds({ 100, 32 });
	this->set_pos({ 100, 50 });
}


Docanto::Geometry::Dimension<long> DocantoWin::UIToolbar::get_min_dims() {
	return Docanto::Geometry::Dimension<long>({ 30, 30 });
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
	namespace Vars = AppVariables::Colors;

	auto bound = get_bounds();
	Docanto::Geometry::Dimension<float> dims = { bound.height, bound.height };
	size_t amount_element = 5;
	auto offset = (bound.width - amount_element * dims.width) / 2.0f;

	render->begin_draw();
	render->clear(Vars::get(Vars::TYPE::PRIMARY_COLOR));
	render->draw_svg(IDR_SVG_MOVE_TOOL  , { offset + dims.width * 0, 0 }, Vars::get(Vars::TYPE::TEXT_COLOR), dims);
	render->draw_svg(IDR_SVG_SELECT_TOOL, { offset + dims.width * 1, 0 }, Vars::get(Vars::TYPE::TEXT_COLOR), dims);
	render->draw_svg(IDR_SVG_ERASE_TOOL , { offset + dims.width * 2, 0 }, Vars::get(Vars::TYPE::TEXT_COLOR), dims);
	render->draw_svg(IDR_SVG_PEN_TOOL   , { offset + dims.width * 3, 0 }, {255, 0, 0}, dims);
	render->draw_svg(IDR_SVG_PEN_TOOL   , { offset + dims.width * 4, 0 }, {255, 255, 0}, dims);
	
	render->end_draw();
}
