#include "TabHandler.h"

void DocantoWin::TabHandler::add(std::shared_ptr<TabContext> ctx) {
	m_all_tabs.push_back(ctx);
}

std::shared_ptr<DocantoWin::TabContext> DocantoWin::TabHandler::get_active_tab() const {
	return m_all_tabs.at(current_tab);
}

void DocantoWin::TabHandler::set_active_tab(size_t id) {
	if (id >= m_all_tabs.size()) {
		return;
	}
	current_tab = id;
}
