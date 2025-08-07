#include "UIDebug.h"
#include "helper/AppVariables.h"
#include "pdf/PDFHandler.h"
#include <format>

DocantoWin::UIDebugElement::UIDebugElement(const std::wstring& UIName, std::weak_ptr<Context> c) : GenericUIObject(UIName, c, true) {
	this->set_bounds({ 100, 100 });
	this->set_pos({ 50, 50 });
}


Docanto::Geometry::Dimension<long> DocantoWin::UIDebugElement::get_min_dims() {
	return Docanto::Geometry::Dimension<long>({ 50, 50 });
}

int DocantoWin::UIDebugElement::hit_test(Docanto::Geometry::Point<long> where) {
	return HTCAPTION;
}

bool DocantoWin::UIDebugElement::pointer_press(const Window::PointerInfo& p, int hit) {
	return false;
}

bool DocantoWin::UIDebugElement::pointer_update(const Window::PointerInfo& p, int hit) {
	return false;
}

bool DocantoWin::UIDebugElement::pointer_release(const Window::PointerInfo& p, int hit) {
	return false;
}


void DocantoWin::UIDebugElement::draw(std::shared_ptr<Direct2DRender> render) {
	float size = 18;
	float padding = 5;

	Docanto::Geometry::Point<float> offset = {};

	render->begin_draw();
	render->clear(AppVariables::Colors::get(AppVariables::Colors::TYPE::PRIMARY_COLOR));
	render->draw_text(L"Debug Information:", offset, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);
	render->draw_text(std::format(L"Scale {:.2f}", this->ctx.lock()->render->get_transform_scale()), 
		{offset.x, offset.y + padding + size}, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);
	
	render->draw_text(std::format(L"Amount of PDF open {:d}", this->ctx.lock()->tabs->get_active_tab()->pdfhandler->get_amount_of_pdfs()),
		{ offset.x, offset.y + (padding + size) * 2 }, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);
	
	render->draw_text(std::format(L"Amount of Tabs {:d}", this->ctx.lock()->tabs->get_all_tabs().size()),
		{ offset.x, offset.y + (padding + size) * 3 }, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);
	
	render->end_draw();
}

DocantoWin::Window::CURSOR_TYPE DocantoWin::UIDebugElement::get_mouse(Docanto::Geometry::Point<float> where) {
	return Window::CURSOR_TYPE::POINTER;
}
