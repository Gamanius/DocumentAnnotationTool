#ifndef _DOCANTOWIN_TABCONTEXT_
#define _DOCANTOWIN_TABCONTEXT_

#include <memory>
#include <vector>

namespace DocantoWin {
	class PDFHandler;
	class ToolHandler;

	struct TabContext {
		std::shared_ptr<PDFHandler> pdfhandler;
		std::shared_ptr<ToolHandler> toolhandler;
	};

	class TabHandler {
		std::vector<std::shared_ptr<TabContext>> m_all_tabs;
		size_t current_tab = 0;
	public:
		TabHandler() = default;

		void add(std::shared_ptr<TabContext> ctx);

		std::shared_ptr<TabContext> get_active_tab() const;
		void set_active_tab(size_t id);

		const std::vector<std::shared_ptr<TabContext>>& get_all_tabs() const;
	};
}

#endif // !_DOCANTOWIN_TABCONTEXT_
