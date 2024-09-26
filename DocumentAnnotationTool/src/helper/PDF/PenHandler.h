#pragma once
#include "../General/General.h"
#include "../Windows/Direct2D.h"


class PenHandler {
public:
	struct Pen {
		float width = 1.0f;
		Renderer::AlphaColor color = { 0, 0, 0, 1 };
	};
private:
	std::vector<Pen> m_pens;
public:

	PenHandler();

	// we load the pens from the json file
	void load_pens();

	void save_pens();

	void select_next_pen();

	const Pen& get_pen() const;
	const std::vector<Pen>& get_all_pens() const;
};

void to_json(nlohmann::json& j, const PenHandler::Pen& p);
void from_json(const nlohmann::json& j, PenHandler::Pen& p);
