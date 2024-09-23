#pragma once
#include "../General/General.h"
#include "../Windows/Direct2D.h"

const std::wstring PEN_SETTINGS_FILE_NAME = std::wstring(L"pen_settings.json"); 

class PenHandler {
public:
	struct Pen {
		float width = 1.0f;
		Renderer::AlphaColor color = { 0, 0, 0, 1 };
	};
private:
	std::vector<Pen> m_pens;
	size_t m_current_pen_index = 0;
public:

	PenHandler();

	// we load the pens from the json file
	void load_pens();

	void save_pens();

	void select_next_pen();

	const Pen& get_pen() const;
	const std::vector<Pen>& get_all_pens() const;

	size_t get_pen_index() const;
};

void to_json(nlohmann::json& j, const PenHandler::Pen& p);
void from_json(const nlohmann::json& j, PenHandler::Pen& p);
