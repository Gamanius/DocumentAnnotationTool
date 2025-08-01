#ifndef _DOCANTOWIN_TOOLHANDLER_H_
#define _DOCANTOWIN_TOOLHANDLER_H_

#include "PDFHandler.h"

namespace DocantoWin {

	class ToolHandler {
	public:
		enum class ToolType {
			SQUARE_SELECTION,
			HAND_MOVEMENT,
			ERASEER,
			PEN,
		};

		struct Tool {
			ToolType type = ToolType::PEN;

			Docanto::Color col;
			float width = 1;
		};
	private:
		std::shared_ptr<PDFHandler> m_pdfhandler;
		std::vector<Tool> m_all_tools;
		size_t m_current_tool_index = 0;
	public:

		ToolHandler(std::shared_ptr<PDFHandler> pdf);
		const std::vector<Tool>& get_all_tools() const;
		const Tool& get_current_tool() const;
		const size_t get_current_tool_index() const;
		void set_current_tool_index(size_t id);
	};

}

#endif
