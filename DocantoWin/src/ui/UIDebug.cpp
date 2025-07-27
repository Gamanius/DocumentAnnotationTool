#include "UIDebug.h"
#include "helper/AppVariables.h"
#include "pdf/PDFHandler.h"
#include <format>

DocantoWin::UIDebugElement::UIDebugElement(const std::wstring& UIName) : GenericUIObject(UIName) {
}

Docanto::Geometry::Dimension<float> DocantoWin::UIDebugElement::get_bounds() {
	return Docanto::Geometry::Dimension<float>({300, 300});
}

Docanto::Geometry::Dimension<long> DocantoWin::UIDebugElement::get_min_dims() {
	return Docanto::Geometry::Dimension<long>({ 300, 300 });
}

int DocantoWin::UIDebugElement::hit_test(Docanto::Geometry::Point<long> where) {
	return HTCAPTION;
}

void DocantoWin::UIDebugElement::pointer_press(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIDebugElement::pointer_update(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIDebugElement::pointer_release(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIDebugElement::draw(std::shared_ptr<Direct2DRender> render) {
	float size = 18;
	float padding = 5;

	auto offset = get_pos();
	auto r = render;
	if (r == nullptr) {
		r = this->m_localrender;
		offset = {};
	}

	r->begin_draw();
	r->clear(AppVariables::Colors::get(AppVariables::Colors::TYPE::PRIMARY_COLOR));
	r->draw_text(L"Debug Information:", offset, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);
	r->draw_text(std::format(L"Scale {:.2f}", this->ctx.lock()->render->get_transform_scale()), 
		{offset.x, offset.y + padding + size}, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);
	
	r->draw_text(std::format(L"Amount of PDF open {:d}", this->ctx.lock()->tabs->get_active_tab()->pdfhandler->get_amount_of_pdfs()),
		{ offset.x, offset.y + (padding + size) * 2 }, AppVariables::Colors::get(AppVariables::Colors::TYPE::TEXT_COLOR), size);

	r->end_draw();
}
