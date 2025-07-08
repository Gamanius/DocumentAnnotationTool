#ifndef _DOCANTOWIN_GESTURE_H_
#define _DOCANTOWIN_GESTURE_H_

#include "win/Direct2D.h"
#include "pdf/PDFHandler.h"

#include <array>

namespace DocantoWin {
	class GestureHandler {
		std::shared_ptr<Direct2DRender> m_render;
		// used for bounds
		std::shared_ptr<PDFHandler> m_pdfhandler;

		struct GestureFinger {
			UINT id = 0;
			bool active = 0;
			Docanto::Geometry::Point<float> initial_position;
			Docanto::Geometry::Point<float> last_position;
		};

		std::array<GestureFinger, 5> m_gesturefinger;
		std::array<GestureFinger, 5> m_touchpad;
		Docanto::Geometry::Point<float> m_initial_offset = { 0, 0 };

		D2D1::Matrix3x2F m_initialScaleMatrix;
		D2D1::Matrix3x2F m_initialScaleMatrixInv;


		void process_one_finger(GestureFinger& finger);
		void process_two_finger(GestureFinger& finger1, GestureFinger& finger2);
	public:
		GestureHandler(std::shared_ptr<Direct2DRender> render, std::shared_ptr<PDFHandler> pdfhandler);

		void start_gesture(const Window::PointerInfo& p);
		void update_gesture(const Window::PointerInfo& p);
		void end_gesture(const Window::PointerInfo& p);

		byte amount_finger_active() const;
		bool is_gesture_active() const;

	};
}

#endif // !_DOCANTOWIN_GESTURE_H_
