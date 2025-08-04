#include "UIPDFManager.h"
#include "helper/AppVariables.h"
#include "pdf/PDFHandler.h"
#include <format>

#include "pdf/ToolHandler.h"

#include "../../rsc/resource.h"

DocantoWin::UIPDFManager::UIPDFManager(const std::wstring& UIName) : GenericUIObject(UIName, true) {
	this->set_bounds({ 100, 100 });
	this->set_pos({ 100, 100 });
}

Docanto::Geometry::Dimension<long> DocantoWin::UIPDFManager::get_min_dims() {
	return Docanto::Geometry::Dimension<long>(100, 100);
}

int DocantoWin::UIPDFManager::hit_test(Docanto::Geometry::Point<long> where) {
	return HTCAPTION;
}

void DocantoWin::UIPDFManager::pointer_press(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIPDFManager::pointer_update(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIPDFManager::pointer_release(Docanto::Geometry::Point<float> where, int hit) {
}

void DocantoWin::UIPDFManager::draw(std::shared_ptr<Direct2DRender> render) {
	namespace Vars = AppVariables::Colors;

	const auto& currenttab = ctx.lock()->tabs->get_active_tab();
	const auto& all_pdfs = currenttab->pdfhandler->get_all_pdfs();

	float box_size = 30;

	render->begin_draw();
	render->clear(Vars::get(Vars::TYPE::PRIMARY_COLOR));

	for (size_t i = 0; i < all_pdfs.size(); i++) {
		render->draw_text(all_pdfs[i].pdf->path.filename(), { 0, float(i * box_size)}, Vars::get(Vars::TYPE::TEXT_COLOR), box_size);
		render->draw_rect_filled({ get_bounds().width - box_size, float(i * box_size) , box_size, box_size}, Vars::get(Vars::TYPE::PRIMARY_COLOR));
		render->draw_svg(IDR_SVG_SAVE_SYMBOL, {get_bounds().width - box_size, float(i * box_size)}, Vars::get(Vars::TYPE::TEXT_COLOR), {box_size, box_size});
	}

	render->end_draw();
}
