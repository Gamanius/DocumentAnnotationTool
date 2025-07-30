#include "ToolHandler.h"

DocantoWin::ToolHandler::ToolHandler(std::shared_ptr<DocantoWin::PDFHandler> pdf) : m_pdfhandler(pdf) {
	m_all_tools.push_back({ ToolType::HAND_MOVEMENT });
	m_all_tools.push_back({ ToolType::SQUARE_SELECTION });
	m_all_tools.push_back({ ToolType::PEN });
}

const std::vector<DocantoWin::ToolHandler::Tool>& DocantoWin::ToolHandler::get_all_tools() {
	return m_all_tools;
}
