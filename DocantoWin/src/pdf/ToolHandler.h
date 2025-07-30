#ifndef _DOCANTOWIN_TOOLHANDLER_H_
#define _DOCANTOWIN_TOOLHANDLER_H_

#include "PDFHandler.h"

namespace DocantoWin {

	class ToolHandler {
	public:
		enum class ToolType {
			SQUARE_SELECTION,
			HAND_MOVEMENT,
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

	public:

		ToolHandler(std::shared_ptr<PDFHandler> pdf);
		const std::vector<Tool>& get_all_tools();
	};

}

#endif
