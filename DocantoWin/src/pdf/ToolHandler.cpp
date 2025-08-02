#include "ToolHandler.h"

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
	m_ink_target = m_pdfhandler->get_pdf_at_point(p);

	if (m_ink_target.first.pdf == nullptr) {
		return;
	}

	m_current_ink.push_back(m_render->inv_transform(p) - m_ink_target.first.render->get_position(m_ink_target.second));
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

	m_current_ink.push_back(m_render->inv_transform(p) - m_ink_target.first.render->get_position(m_ink_target.second));
}

void DocantoWin::ToolHandler::end_ink(Docanto::Geometry::Point<float> p) {
	if (m_ink_target.first.pdf == nullptr) {
		return;
	}
	m_current_ink.push_back(m_render->inv_transform(p) - m_ink_target.first.render->get_position(m_ink_target.second));

	if (m_ink_target.first.annotation != nullptr and m_current_ink.size() > 2) {
		m_ink_target.first.annotation->add_annotation(m_ink_target.second, 
			m_current_ink, get_current_tool().col, get_current_tool().width);

		m_ink_target.first.render->reload_annotations_page(m_ink_target.second);

		m_render->get_attached_window()->send_paint_request();
	}

	m_current_ink.clear();
}

void DocantoWin::ToolHandler::draw() {
	if (m_current_ink.size() <= 2) {
		return;
	}

	if (m_ink_target.first.render == nullptr) {
		return;
	}

	m_render->begin_draw();


	for (size_t i = 1; i < m_current_ink.size(); i++) {
		m_render->draw_line(m_current_ink[i - 1] + m_ink_target.first.render->get_position(m_ink_target.second), m_current_ink[i] + m_ink_target.first.render->get_position(m_ink_target.second), get_current_tool().col, get_current_tool().width);
	}

	m_render->end_draw();
}