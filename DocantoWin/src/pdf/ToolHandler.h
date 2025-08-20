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
		std::shared_ptr<Direct2DRender> m_render;
		std::vector<Tool> m_all_tools;
		size_t m_current_tool_index = 0;

		std::vector<Docanto::Geometry::Point<float>> m_current_ink;
		std::pair<PDFHandler::PDFWrapper, size_t> m_pdf_target;

		std::optional<Docanto::Geometry::Point<float>> m_selection_start = std::nullopt;
		Docanto::Geometry::Point<float> m_selection_last_point;
		std::vector<std::shared_ptr<Docanto::PDFAnnotation::AnnotationInfo>> m_selection_annotations;
	public:

		ToolHandler(std::shared_ptr<PDFHandler> pdf, std::shared_ptr<Direct2DRender>r);
		const std::vector<Tool>& get_all_tools() const;
		const Tool& get_current_tool() const;
		const size_t get_current_tool_index() const;
		void set_current_tool_index(size_t id);
		
		void start_ink(Docanto::Geometry::Point<float> p);
		void update_ink(Docanto::Geometry::Point<float> p);
		void end_ink(Docanto::Geometry::Point<float> p);
		
		void start_eraser(Docanto::Geometry::Point<float> p);
		void update_eraser(Docanto::Geometry::Point<float> p);
		void end_eraser(Docanto::Geometry::Point<float> p);
		
		void start_square_selection(Docanto::Geometry::Point<float> p);
		void update_square_selection(Docanto::Geometry::Point<float> p);
		void end_square_selection(Docanto::Geometry::Point<float> p);

		void selection_remove_from_pdf();

		void draw();
	};

}

#endif
