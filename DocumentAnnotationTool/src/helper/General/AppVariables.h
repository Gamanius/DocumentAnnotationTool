#pragma once

#include "json.hpp"
#include "FileHandler.h"
#include "Renderer.h"

#undef COLOR_BACKGROUND 

inline std::function<void()> ABNORMAL_PROGRAM_EXIT_CALLBACK; 

namespace SessionVariables {
	inline std::filesystem::path FILE_PATH;
	inline std::wstring WINDOW_TITLE = APPLICATION_NAME; 
	inline size_t PENSELECTION_SELECTED_PEN = 0; 

	inline bool PDF_UNSAVED_CHANGES = false;
}

namespace AppVariables {
	inline const std::wstring APPVARIABLES_SETTINGS_FILE_NAME = std::wstring(L"app_settings.json");
	inline size_t             APPVARIABLES_MAX_LOG_FILES = 20;
	inline size_t             APPVARIABLES_REFRESHTIME_MS = 10;

	inline float        WINDOWLAYOUT_TOOLBAR_HEIGHT     = 24.0f;
	inline std::wstring WINDOWLAYOUT_FONT               = L"Courier New";
	
	inline float        PENSELECTION_Y_POSITION         = 10.0f;
	inline float        PENSELECTION_PENS_WIDTH         = 35.0f;
	inline std::wstring PENSELECTION_SETTINGS_FILE_NAME = std::wstring(L"pen_settings.json");

	inline std::wstring PDF_TEMPLATE_FOLDER_NAME = std::wstring(L"templates");
	inline float		PDF_SEPERATION_DISTANCE = 10.0f;
		
	inline Renderer::Color COLOR_BACKGROUND             = { 50, 50, 50 };
	inline Renderer::Color COLOR_TEXT                   = { 255, 255, 255 };

	inline Renderer::Color COLOR_PRIMARY                = { 33, 118, 255 };
	inline Renderer::Color COLOR_SECONDARY              = { 25, 183, 240 };
	inline Renderer::Color COLOR_TERTIARY               = { 0, 214, 164 };

	inline float CONRTOLS_MOUSE_ZOOM_SCALE              = 1.25f;

	void load();
	void save();
}