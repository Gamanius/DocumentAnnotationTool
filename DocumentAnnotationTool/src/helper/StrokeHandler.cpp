#include "include.h"

StrokeHandler::Stroke::Stroke(Stroke&& s) noexcept {
	swap(*this, s);
}

StrokeHandler::Stroke& StrokeHandler::Stroke::operator=(Stroke s) noexcept {
	swap(*this, s);
	return *this;
}

StrokeHandler::StrokeHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window) : m_pdf(pdf), m_renderer(renderer), m_window(window ){

}

StrokeHandler::StrokeHandler(StrokeHandler&& t) noexcept {
	swap(*this, t);
}

StrokeHandler& StrokeHandler::operator=(StrokeHandler t) noexcept {
	swap(*this, t);
	return *this;
}

StrokeHandler::~StrokeHandler() {
}

void StrokeHandler::start_stroke(const WindowHandler::PointerInfo& p) {
	// First check if the stroke with the id already exists
	ASSERT(m_active_strokes.find(p.id) == m_active_strokes.end(), "Trying to start a new ink stroke with a pen that is already drawing a stroke");

	// add a new strok to the map
	Stroke s;
	s.points.push_back(m_renderer->inv_transform_point(p.pos));

	m_active_strokes[p.id] = std::move(s);
}

void StrokeHandler::update_stroke(const WindowHandler::PointerInfo& p) {
	// First check if the stroke with the id already exists
	// and ignore it if it doesn't
	if (m_active_strokes.find(p.id) == m_active_strokes.end()) {
		return;
	}

	m_active_strokes[p.id].points.push_back(m_renderer->inv_transform_point(p.pos));
}

void StrokeHandler::end_stroke(const WindowHandler::PointerInfo& p) {
	// First check if the stroke with the id already exists
	ASSERT(m_active_strokes.find(p.id) != m_active_strokes.end(), "Trying to end a ink stroke with a pen that was never drawing a stroke in the first place");

	// update for the last time
	update_stroke(p);
	m_active_strokes.at(p.id).geometry = Renderer::create_bezier_geometry(m_active_strokes.at(p.id).points); // create the bezier geometry
	m_active_strokes.at(p.id).path = m_renderer->create_bezier_path(m_active_strokes.at(p.id).geometry);
	// add the stroke to the list of strokes
	m_strokes.push_back(std::move(m_active_strokes[p.id]));
	m_active_strokes.erase(p.id);
}

void StrokeHandler::render_strokes() {
	// iterate through all strokes and draw them using the renderer
	m_renderer->set_current_transform_active();
	m_renderer->begin_draw();
	for (size_t i = 0; i < m_strokes.size(); i++) {
		m_renderer->draw_path(m_strokes.at(i).path, m_strokes.at(i).color, m_strokes.at(i).thickness); 
	}

	// also do it for the active ones
	for (auto& [id, stroke] : m_active_strokes) {
		for (size_t j = 0; j < stroke.points.size() - 1; j++) {
			m_renderer->draw_line(stroke.points.at(j), stroke.points.at(j + 1), stroke.color, stroke.thickness);
		}
	}
	m_renderer->end_draw();
}

void swap(StrokeHandler& first, StrokeHandler& second) {
	using std::swap;

	swap(first.m_pdf, second.m_pdf);
	swap(first.m_renderer, second.m_renderer);
	swap(first.m_window, second.m_window);

	swap(first.m_strokes, second.m_strokes);
}
