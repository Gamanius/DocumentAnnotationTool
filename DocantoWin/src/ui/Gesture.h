#ifndef _DOCANTOWIN_GESTURE_H_
#define _DOCANTOWIN_GESTURE_H_

#include "win/Direct2D.h"
#include "pdf/PDFHandler.h"

#include "helper/AppVariables.h"
#include "helper/TabHandler.h"

#include <array>

namespace DocantoWin {
	/// <summary>
	/// This class is used for any Finger/Touchpad gestures like panning, zomming and rotating
	/// 
	/// TODO:
	///		- The rotating with the fingers should snap to n * 90 deg
	///		- Maybe add touchpad rotation?
	///		- Limit the amount of zoom to and upper and lower bound 
	/// </summary>
	class GestureHandler {
		std::shared_ptr<Direct2DRender> m_render;
		// used for bounds
		std::shared_ptr<TabHandler> m_tabcontext;

		struct GestureFinger {
			UINT id = 0;
			bool active = 0;
			Docanto::Geometry::Point<float> initial_position;
			Docanto::Geometry::Point<float> last_position;
		};

		std::array<GestureFinger, 5> m_gesturefinger;
		Docanto::Geometry::Point<float> m_initial_offset = { 0, 0 };

		bool m_isGestureTouchpad = false;
		bool m_touchpadWasOneFingerActive = true;
		bool m_touchpadMovedFar = false;

		D2D1::Matrix3x2F m_initialTranslationMatrix;
		D2D1::Matrix3x2F m_initialTransformMatrixInv;

		D2D1::Matrix3x2F m_initialRotationMatrix;
		D2D1::Matrix3x2F m_initialRotationMatrixInv;

		D2D1::Matrix3x2F m_initialScaleMatrix;
		D2D1::Matrix3x2F m_initialScaleMatrixInv;


		void process_one_finger(GestureFinger& finger);
		void process_two_finger(GestureFinger& finger1, GestureFinger& finger2);
		void update_local_matrices();
	public:
		GestureHandler(std::shared_ptr<Direct2DRender> render, std::shared_ptr<TabHandler> pdfhandler);

		void start_gesture(const Window::PointerInfo& p);
		void update_gesture(const Window::PointerInfo& p);
		void end_gesture(const Window::PointerInfo& p);

		byte amount_finger_active() const;
		bool is_gesture_active() const;

	};
}

#endif // !_DOCANTOWIN_GESTURE_H_
