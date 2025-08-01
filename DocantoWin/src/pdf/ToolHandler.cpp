#include "ToolHandler.h"

DocantoWin::ToolHandler::ToolHandler(std::shared_ptr<DocantoWin::PDFHandler> pdf) : m_pdfhandler(pdf) {
	m_all_tools.push_back({ ToolType::HAND_MOVEMENT });
	m_all_tools.push_back({ ToolType::SQUARE_SELECTION });
	m_all_tools.push_back({ ToolType::ERASEER });
	m_all_tools.push_back({ ToolType::PEN, {255, 0, 0} });
	m_all_tools.push_back({ ToolType::PEN, {0, 255, 0} });
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
