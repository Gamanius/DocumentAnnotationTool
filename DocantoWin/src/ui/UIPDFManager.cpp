#include "include.h"

#include "UIPDFManager.h"

#include "helper/TabHandler.h"
#include "helper/AppVariables.h"


#include "pdf/PDFHandler.h"

#include <format>


#include "pdf/ToolHandler.h"

#include "resource.h"

std::optional<std::wstring> temp_save_file_dialog(const wchar_t* filter, HWND windowhandle = nullptr) {
	OPENFILENAME ofn;
	WCHAR szFile[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = windowhandle; // If you have a window handle, specify it here.
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn)) {
		return std::wstring(ofn.lpstrFile);
	}
	auto error = CommDlgExtendedError();
	return std::nullopt;
}


std::vector<Docanto::Geometry::Rectangle<float>> DocantoWin::UIPDFManager::get_save_recs() {
	const auto& currenttab = this->ctx.lock()->tabs->get_active_tab();
	const auto amount_of_pdf = currenttab->pdfhandler->get_all_pdfs().size();
	
	std::vector<Docanto::Geometry::Rectangle<float>> recs(amount_of_pdf);

	for (size_t i = 0; i < amount_of_pdf; i++) {
		recs[i] = { get_bounds().width - box_size, float(i * box_size), box_size, box_size };
	}

	return recs;
}

void DocantoWin::UIPDFManager::update_hovering() {
	auto recs = get_save_recs();
	for (size_t i = 0; i < recs.size(); i++) {
		if (recs[i].intersects(this->get_mouse_pos())) {
			currently_hovering = i;
			return;
		}
	}
}

DocantoWin::UIPDFManager::UIPDFManager(std::weak_ptr<Context> c, const std::wstring& UIName) : GenericUIObject(UIName, c) {
	this->set_bounds({ 100, 100 });
	this->set_pos({ 100, 100 });
}

Docanto::Geometry::Dimension<long> DocantoWin::UIPDFManager::get_min_dims() {
	return Docanto::Geometry::Dimension<long>(100, 100);
}

int DocantoWin::UIPDFManager::hit_test(Docanto::Geometry::Point<long> where) {
	auto recs = get_save_recs();
	for (size_t i = 0; i < recs.size(); i++) {
		if (recs[i].intersects(where)) {
			return HTCLIENT;
		}
	}
	return HTCAPTION;
}

bool DocantoWin::UIPDFManager::pointer_press(const Window::PointerInfo& p, int hit) {
	return false;
}

bool DocantoWin::UIPDFManager::pointer_update(const Window::PointerInfo& p, int hit) {
	auto recs = get_save_recs();
	for (size_t i = 0; i < recs.size(); i++) {
		if (recs[i].intersects(p.pos)) {
			currently_hovering = i;
			return true;
		}
	}

	if (currently_hovering == ~0) {
		return false;
	}

	currently_hovering = ~0;
	return true;
}

bool DocantoWin::UIPDFManager::pointer_release(const Window::PointerInfo& p, int hit) {
	auto recs = get_save_recs();
	for (size_t i = 0; i < recs.size(); i++) {
		if (recs[i].intersects(p.pos)) {
			const auto& currenttab = this->ctx.lock()->tabs->get_active_tab();
			const auto& all_pdfs = currenttab->pdfhandler->get_all_pdfs();

			// get save dat
			auto path = temp_save_file_dialog(L"PDF\0*.pdf\0\0", ctx.lock()->window->get_hwnd());
			if (!path.has_value()) {
				return false;
			}

			all_pdfs.at(i).pdf->save(path.value());

			return true;
		}
	}

	return false;
}

void DocantoWin::UIPDFManager::draw(std::shared_ptr<Direct2DRender> render) {
	namespace Vars = AppVariables::Colors;

	const auto& currenttab = this->ctx.lock()->tabs->get_active_tab();
	const auto& all_pdfs = currenttab->pdfhandler->get_all_pdfs();

	auto recs = get_save_recs();

	update_hovering();

	render->begin_draw();
	render->clear(Vars::get(Vars::TYPE::PRIMARY_COLOR));

	for (size_t i = 0; i < all_pdfs.size(); i++) {
		render->draw_text(all_pdfs[i].pdf->path.filename(), { 0, float(i * box_size)}, Vars::get(Vars::TYPE::TEXT_COLOR), box_size);
		render->draw_rect_filled(recs[i], Vars::get(Vars::TYPE::PRIMARY_COLOR));

		if (i == currently_hovering) {
			render->draw_svg(IDR_SVG_SAVE_SYMBOL, recs[i].upperleft(), Vars::get(Vars::TYPE::ACCENT_COLOR), recs[i].dims());
		}
		else {
			render->draw_svg(IDR_SVG_SAVE_SYMBOL, recs[i].upperleft(), Vars::get(Vars::TYPE::TEXT_COLOR), recs[i].dims());
		}
	}

	render->end_draw();
}

DocantoWin::Window::CURSOR_TYPE DocantoWin::UIPDFManager::get_mouse(Docanto::Geometry::Point<float> where) {
	return Window::CURSOR_TYPE::NONE;
}
