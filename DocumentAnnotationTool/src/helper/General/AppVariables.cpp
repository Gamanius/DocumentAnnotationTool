#include "AppVariables.h"

#include <codecvt> 
#include <locale>  

using json = nlohmann::json;

std::string to_utf8(const std::wstring& wide_string) {
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
	return utf8_conv.to_bytes(wide_string);
}

template <typename T>
void json_save(json& j, std::string field, T val) {
	j[field] = val;
}

void json_save(json& j, std::string field, const std::wstring& val) {
	j[field] = to_utf8(val);
}

template <typename T>
void json_load(json& j, std::string field, T& val) {
	val = j[field].get<T>();
}

void json_load(json& j, std::string field, std::wstring& val) {
	std::string s = j[field].get<std::string>(); 
	val = std::wstring(s.begin(), s.end());
}

#define JSON_SAVE(json, var) json_save(json, #var, var); 
#define JSON_LOAD(json, var) json_load(json, #var, var);


void AppVariables::load() {
	auto settings = FileHandler::open_file_appdata(APPVARIABLES_SETTINGS_FILE_NAME);

	if (settings.has_value() == false) {
	JSON_PARSE_ERROR:
		Logger::log("Generating default setting json.");
		// Settings file may be faulty. lets resave it
		save();
		return;
	}

	json j;
	try {
		j = json::parse(reinterpret_cast<char*>(settings->data), reinterpret_cast<char*>(settings->data) + settings->size);

		JSON_LOAD(j, APPVARIABLES_MAX_LOG_FILES);
		JSON_LOAD(j, APPVARIABLES_REFRESHTIME_MS);

		JSON_LOAD(j, WINDOWLAYOUT_TOOLBAR_HEIGHT);
		JSON_LOAD(j, WINDOWLAYOUT_FONT);
		
		JSON_LOAD(j, PENSELECTION_Y_POSITION);
		JSON_LOAD(j, PENSELECTION_PENS_WIDTH);
		JSON_LOAD(j, PENSELECTION_SETTINGS_FILE_NAME);

		JSON_LOAD(j, PDF_TEMPLATE_FOLDER_NAME);
		JSON_LOAD(j, PDF_SEPERATION_DISTANCE);
		
		JSON_LOAD(j, COLOR_BACKGROUND);
		JSON_LOAD(j, COLOR_TEXT);

		JSON_LOAD(j, COLOR_PRIMARY);
		JSON_LOAD(j, COLOR_SECONDARY);
		JSON_LOAD(j, COLOR_TERTIARY);
		
		JSON_LOAD(j, CONRTOLS_MOUSE_ZOOM_SCALE);
		JSON_LOAD(j, CONTROLS_ARROWS_OFFSET);
	}
	catch (json::exception& e) { 
		Logger::error("Could not parse settings file: ", e.what()); 
		goto JSON_PARSE_ERROR; 
	}

	Logger::success("Loaded settings from ", APPVARIABLES_SETTINGS_FILE_NAME); 
}

void AppVariables::save() {
	json j;

	JSON_SAVE(j, APPVARIABLES_MAX_LOG_FILES);
	JSON_SAVE(j, APPVARIABLES_REFRESHTIME_MS);

    JSON_SAVE(j, WINDOWLAYOUT_TOOLBAR_HEIGHT); 
    JSON_SAVE(j, WINDOWLAYOUT_FONT);

    JSON_SAVE(j, PENSELECTION_Y_POSITION);
    JSON_SAVE(j, PENSELECTION_PENS_WIDTH);
    JSON_SAVE(j, PENSELECTION_SETTINGS_FILE_NAME);

	JSON_SAVE(j, PDF_TEMPLATE_FOLDER_NAME);
	JSON_SAVE(j, PDF_SEPERATION_DISTANCE);

    JSON_SAVE(j, COLOR_BACKGROUND);
    JSON_SAVE(j, COLOR_TEXT);
    JSON_SAVE(j, COLOR_PRIMARY);
    JSON_SAVE(j, COLOR_SECONDARY);
    JSON_SAVE(j, COLOR_TERTIARY);

    JSON_SAVE(j, CONRTOLS_MOUSE_ZOOM_SCALE);
    JSON_SAVE(j, CONTROLS_ARROWS_OFFSET);

	FileHandler::File f;
	std::string data = j.dump(4);
	f.data = reinterpret_cast<byte*>(&data[0]);
	f.size = data.size();

	FileHandler::write_file_to_appdata(f, AppVariables::APPVARIABLES_SETTINGS_FILE_NAME, true); 

	// file should not delete the data because it is a pointer to a string
	f.data = nullptr;
}
