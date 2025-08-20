#include "include.h"
#include "ToolHandler.h"
#include "helper/AppVariables.h"

DocantoWin::ToolHandler::ToolHandler(std::shared_ptr<DocantoWin::PDFHandler> pdf, std::shared_ptr<Direct2DRender> r) : m_pdfhandler(pdf), m_render(r) {
	m_all_tools.push_back({ ToolType::HAND_MOVEMENT });
	m_all_tools.push_back({ ToolType::SQUARE_SELECTION });
	m_all_tools.push_back({ ToolType::ERASEER });
	m_all_tools.push_back({ ToolType::PEN, {255, 0, 0} });
	m_all_tools.push_back({ ToolType::PEN, {0, 255, 0}, 4 });
	m_all_tools.push_back({ ToolType::PEN, {0, 0, 255} });
}


const std::vector<DocantoWin::ToolHandler::Tool>& DocantoWin::ToolHandler::get_all_tools() const {
	return m_all_tools;
}

const DocantoWin::ToolHandler::Tool& DocantoWin::ToolHandler::get_current_tool() const {
	return m_all_tools[m_current_tool_index];
}

const size_t DocantoWin::ToolHandler::get_current_tool_index() const {
	return m_current_tool_index;
}

void DocantoWin::ToolHandler::set_current_tool_index(size_t id) {
	if (id >= m_all_tools.size()) {
		return;
	}
	m_current_tool_index = id;
}

void DocantoWin::ToolHandler::start_ink(Docanto::Geometry::Point<float> p) {
	m_current_ink.clear();
	m_pdf_target = m_pdfhandler->get_pdf_at_point(p);

	if (m_pdf_target.first.pdf == nullptr) {
		return;
	}

	m_current_ink.push_back(m_render->inv_transform(p) - m_pdf_target.first.render->get_position(m_pdf_target.second));
}

void DocantoWin::ToolHandler::update_ink(Docanto::Geometry::Point<float> p) {
	if (m_current_ink.size() == 0) {
		return;
	}
	auto check_target = m_pdfhandler->get_pdf_at_point(p);
	if (check_target.first.pdf == nullptr) {
		end_ink(p);
		return;
	}

	m_current_ink.push_back(m_render->inv_transform(p) - m_pdf_target.first.render->get_position(m_pdf_target.second));
}

void DocantoWin::ToolHandler::end_ink(Docanto::Geometry::Point<float> p) {
	if (m_pdf_target.first.pdf == nullptr) {
		return;
	}
	m_current_ink.push_back(m_render->inv_transform(p) - m_pdf_target.first.render->get_position(m_pdf_target.second));

	if (m_pdf_target.first.annotation != nullptr and m_current_ink.size() > 2) {
		m_pdf_target.first.annotation->add_annotation(m_pdf_target.second, 
			m_current_ink, get_current_tool().col, get_current_tool().width);

		m_pdf_target.first.render->reload_annotations_page(m_pdf_target.second);

		m_render->get_attached_window()->send_paint_request();
	}

	m_current_ink.clear();
}

void DocantoWin::ToolHandler::start_square_selection(Docanto::Geometry::Point<float> p) {
	m_selection_annotations.clear();
	m_pdf_target = m_pdfhandler->get_pdf_at_point(p);

	if (m_pdf_target.first.pdf == nullptr) {
		return;
	}


	m_selection_start = m_render->inv_transform(p);
	m_selection_last_point = m_render->inv_transform(p);
}

void DocantoWin::ToolHandler::update_square_selection(Docanto::Geometry::Point<float> p) {
	if (!m_selection_start.has_value()) {
		return;
	}

	m_selection_annotations = m_pdf_target.first.annotation->get_annotation(
		m_pdf_target.second, 
		Docanto::Geometry::Rectangle<float>(
			m_selection_start.value() - m_pdf_target.first.render->get_position(m_pdf_target.second),
			m_render->inv_transform(p) - m_pdf_target.first.render->get_position(m_pdf_target.second)).validate()
	);
	m_selection_last_point = m_render->inv_transform(p);

}

void DocantoWin::ToolHandler::end_square_selection(Docanto::Geometry::Point<float> p) {
	m_selection_start.reset();
}

void DocantoWin::ToolHandler::selection_remove_from_pdf() {
	if (m_pdf_target.first.pdf == nullptr) {
		return;
	}

	if (m_selection_annotations.size() == 0) {
		return;
	}

	for (size_t i = 0; i < m_selection_annotations.size(); i++) {
		m_pdf_target.first.annotation->remove_annotation(m_selection_annotations[i]);
	}
	m_selection_annotations.clear();
	m_pdf_target.first.render->reload_annotations_page(m_pdf_target.second);
	m_render->get_attached_window()->send_paint_request();
}

void DocantoWin::ToolHandler::draw() {
	if (m_pdf_target.first.render == nullptr) {
		return;
	}

	if (m_selection_start.has_value()) {
		m_render->draw_rect({ m_selection_start.value(), m_selection_last_point }, AppVariables::Colors::get(AppVariables::Colors::TYPE::SECONDARY_COLOR));
	}

	for (size_t i = 0; i < m_selection_annotations.size(); i++) {
		m_render->draw_rect(m_selection_annotations[i]->bounding_box + m_pdf_target.first.render->get_position(m_pdf_target.second), AppVariables::Colors::get(AppVariables::Colors::TYPE::ACCENT_COLOR));
	}

	if (m_current_ink.size() <= 2) {
		return;
	}

	m_render->begin_draw();


	for (size_t i = 1; i < m_current_ink.size(); i++) {
		m_render->draw_line(m_current_ink[i - 1] + m_pdf_target.first.render->get_position(m_pdf_target.second), m_current_ink[i] + m_pdf_target.first.render->get_position(m_pdf_target.second), get_current_tool().col, get_current_tool().width);
	}

	m_render->end_draw();
}