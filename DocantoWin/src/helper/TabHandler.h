#ifndef _DOCANTOWIN_TABCONTEXT_
#define _DOCANTOWIN_TABCONTEXT_

#include "../include.h"

namespace DocantoWin {
	class PDFHandler;

	struct TabContext {
		std::shared_ptr<PDFHandler> pdfhandler;
	};

	class TabHandler {
		std::vector<std::shared_ptr<TabContext>> m_all_tabs;
		size_t current_tab = 0;
	public:
		TabHandler() = default;

		void add(std::shared_ptr<TabContext> ctx);

		std::shared_ptr<TabContext> get_active_tab() const;
		void set_active_tab(size_t id);
	};
}

#endif // !_DOCANTOWIN_TABCONTEXT_
