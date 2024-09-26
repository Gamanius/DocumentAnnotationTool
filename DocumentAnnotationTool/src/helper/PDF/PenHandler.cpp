#include "PenHandler.h"
using json = nlohmann::json;

void to_json(json& j, const PenHandler::Pen& p) {
	j = json{ {"width", p.width}, {"color", p.color} };
}

void from_json(const json& j, PenHandler::Pen& p) {
	j.at("width").get_to(p.width);
	j.at("color").get_to(p.color);
}

PenHandler::PenHandler() {
	load_pens();
}

void PenHandler::load_pens() {
	auto settings = FileHandler::open_file_appdata(AppVariables::PENSELECTION_SETTINGS_FILE_NAME);

	if (settings.has_value() == false) {
	JSON_PARSE_ERROR:
		Logger::log("Generating new pen settings file with default settings.");
		// There are no pens to load 
		// because the file does not exist. just generate a new one
		m_pens.push_back({ 1.0f, {0, 0, 0, 1} });
		save_pens();
		return;
	}

	json j; 
	try {
		j = json::parse(reinterpret_cast<char*>(settings->data), reinterpret_cast<char*>(settings->data) + settings->size);
		m_pens = j.at("Pens").get<std::vector<Pen>>(); 
	}
	catch (json::exception& e) {
		Logger::error("Could not parse pen settings file: ", e.what());
		goto JSON_PARSE_ERROR;
	}

	Logger::success("Loaded ", m_pens.size(), " pens from settings file.");
}

void PenHandler::save_pens() { 
	json j;
	j["Pens"] = m_pens;
	FileHandler::File f;
	std::string data = j.dump(4); 
	f.data = reinterpret_cast<byte*>(&data[0]);
	f.size = data.size();

	FileHandler::write_file_to_appdata(f, AppVariables::PENSELECTION_SETTINGS_FILE_NAME, true);

	// file should not delete the data because it is a pointer to a string
	f.data = nullptr;
}

void PenHandler::select_next_pen() {
	SessionVariables::PENSELECTION_SELECTED_PEN = (SessionVariables::PENSELECTION_SELECTED_PEN + 1) % m_pens.size();
}

const PenHandler::Pen& PenHandler::get_pen() const {
	return m_pens[SessionVariables::PENSELECTION_SELECTED_PEN];
}

const std::vector<PenHandler::Pen>& PenHandler::get_all_pens() const {
	return m_pens; 
}