#ifndef _DOCANTOWIN_APPVARS_H_
#define _DOCANTOWIN_APPVARS_H_

#include "../include.h"

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
}

#endif // !_DOCANTOWIN_APPVARS_H_
