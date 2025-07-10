#include "AppVariables.h"

Docanto::Color DocantoWin::AppVariables::Colors::get(DocantoWin::AppVariables::Colors::TYPE t) {
	if (isDarkTheme) {
		return ThemeColors[t].second;
	}
	return ThemeColors[t].first;
}