#include "UIToolbar.h"
#include "helper/AppVariables.h"
#include "pdf/PDFHandler.h"
#include <format>

#include "pdf/ToolHandler.h"

#include "../../rsc/resource.h"

std::vector<Docanto::Geometry::Rectangle<float>> DocantoWin::UIToolbar::get_toolbar_recs() {
	const auto& all_tools = ctx.lock()->tabs->get_active_tab()->toolhandler->get_all_tools();
	size_t amount_element = all_tools.size();
	auto bound = get_bounds(); 
	
	Docanto::Geometry::Dimension<float> dims = { bound.height, bound.height };
	auto offset = (bound.width - amount_element * dims.width) / 2.0f;

	std::vector<Docanto::Geometry::Rectangle<float>> recs(amount_element);
	for (size_t i = 0; i < amount_element; i++) {
		recs[i] = { offset + dims.width * i, 0, dims };
	}

	return recs;
}

DocantoWin::UIToolbar::UIToolbar(const std::wstring& UIName) : GenericUIObject(UIName, true) {
	this->set_bounds({ 100, 32 });
	this->set_pos({ 100, 50 });
}


Docanto::Geometry::Dimension<long> DocantoWin::UIToolbar::get_min_dims() {
	if (ctx.lock()  == nullptr) {
		return Docanto::Geometry::Dimension<long>({ 30 * 4, 30 });
	}
	const auto& all_tools = ctx.lock()->tabs->get_active_tab()->toolhandler->get_all_tools();
	long amount_element = all_tools.size();

	return Docanto::Geometry::Dimension<long>({ 30 * (amount_element + 2), 30 });
}

int DocantoWin::UIToolbar::hit_test(Docanto::Geometry::Point<long> where) {
	auto recs = get_toolbar_recs();

	for (size_t i = 0; i < recs.size(); i++) {
		auto r = recs[i];

		if (r.intersects(where)) {

			return HTCLIENT;
		}
	}
	return HTCAPTION;
}

void DocantoWin::UIToolbar::pointer_press(Docanto::Geometry::Point<float> where, int hit) {
	auto recs = get_toolbar_recs();
	const auto& all_tools = ctx.lock()->tabs->get_active_tab()->toolhandler->get_all_tools();

	for (size_t i = 0; i < recs.size(); i++) {
		auto r = recs[i];

		if (r.intersects(where)) {
			if (all_tools[i].type == ToolHandler::ToolType::HAND_MOVEMENT) {
				this->ctx.lock()->window->set_global_cursor(Window::CURSOR_TYPE::HAND);
			}
			else {
				this->ctx.lock()->window->set_default_cursor();
			}

			ctx.lock()->tabs->get_active_tab()->toolhandler->set_current_tool_index(i);
			ctx.lock()->window->send_paint_request();
			break;
		}
	}
}

void DocantoWin::UIToolbar::pointer_update(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIToolbar::pointer_release(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIToolbar::draw(std::shared_ptr<Direct2DRender> render) {
	namespace Vars = AppVariables::Colors;

	const auto& all_tools = ctx.lock()->tabs->get_active_tab()->toolhandler->get_all_tools();
	auto recs = get_toolbar_recs();


	render->begin_draw();
	render->clear(Vars::get(Vars::TYPE::PRIMARY_COLOR));

	for (size_t i = 0; i < recs.size(); i++) {
		auto r = recs[i];
		auto tool = all_tools[i];

		size_t id_to_draw = IDR_SVG_MOVE_TOOL;
		Docanto::Color c = Vars::get(Vars::TYPE::TEXT_COLOR);

		switch (tool.type) {
		case ToolHandler::ToolType::HAND_MOVEMENT:
		{
			id_to_draw = IDR_SVG_MOVE_TOOL;
			break;
		}
		case ToolHandler::ToolType::ERASEER:
		{
			id_to_draw = IDR_SVG_ERASE_TOOL;
			break;
		}
		case ToolHandler::ToolType::PEN:
		{
			id_to_draw = IDR_SVG_PEN_TOOL;
			c = tool.col;
			break;
		}
		case ToolHandler::ToolType::SQUARE_SELECTION:
		{
			id_to_draw = IDR_SVG_SELECT_TOOL;
			break;
		}
		}
		
		if (i == ctx.lock()->tabs->get_active_tab()->toolhandler->get_current_tool_index()) {
			render->draw_rect_filled(r, Vars::get(Vars::TYPE::ACCENT_COLOR));
		}
		render->draw_svg(id_to_draw, r.upperleft(), c, r.dims());

	}

	
	render->end_draw();
}
