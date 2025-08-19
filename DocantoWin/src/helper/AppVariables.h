#ifndef _DOCANTOWIN_APPVARS_H_
#define _DOCANTOWIN_APPVARS_H_

#include <map>
#include "general/BasicRender.h"

namespace DocantoWin::AppVariables {
	// Control elements

	// this is the threshold for zooming. if the fingers distance exceed the amount it will move over to a zoom
	inline float TOUCHPAD_DISTANCE_ZOOM_THRESHOLD = 100.0f;
	// if the center moves more than the distance it cannot go over to a zoom anymore
	inline float TOUCHPAD_MIN_TRAVEL_DISTANCE = 150.0f;
	// this distance is needed at least so it counts as a pan/zoom
	inline float TOUCHPAD_JITTER_DISTANCE = 50.0f;
	// This controls how sensitive the panning is with the touchpad
	inline float TOUCHPAD_PAN_SCALE_FACTOR = 0.7f;

	// if set to false you cannot rotate using finger gestures
	inline bool  TOUCH_ALLOW_ROTATION = false;

	namespace Colors {
		// This needs to be updated manually (by the Window class since it knows that)
		inline bool isDarkTheme = false;

		enum class TYPE {
			BACKGROUND_COLOR,
			TEXT_COLOR,
			PRIMARY_COLOR,
			SECONDARY_COLOR,
			ACCENT_COLOR,
		};

		//              Index           Light theme     Dark theme
		inline std::map<TYPE, std::pair<Docanto::Color, Docanto::Color>> ThemeColors = {
			{TYPE::BACKGROUND_COLOR, {{212, 212, 215}, {32 , 32 , 32 }}},
			{TYPE::TEXT_COLOR,       {{18 , 18 , 18 }, {255, 249, 241}}},
			{TYPE::PRIMARY_COLOR,    {{235, 235, 239}, {57 , 138, 219}}},
			{TYPE::SECONDARY_COLOR,  {{59 , 196, 132}, {29 , 89 , 143}}},
			{TYPE::ACCENT_COLOR,     {{255, 211, 51 }, {200, 0  , 255}}}
		};

		Docanto::Color get(TYPE);
	}

	inline size_t WINDOW_PAINT_REFRESH_TIME_MS = 16;

	inline float RENDER_MAX_SCALE = 150;
	inline float RENDER_MIN_SCALE = 0.01;

	inline float UI_ELEMENTS_TEXT_SIZE = 20.0f;
	inline float UI_ELEMENTS_PADDING = 5.0f;
	inline float UI_ELEMENTS_MARGINS = 5.0f;
}

#endif // !_DOCANTOWIN_APPVARS_H_
