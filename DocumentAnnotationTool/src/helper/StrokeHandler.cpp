#include "include.h"

StrokeHandler::Stroke::Stroke(Stroke&& s) noexcept {
	swap(*this, s);
}

StrokeHandler::Stroke& StrokeHandler::Stroke::operator=(Stroke s) noexcept {
	swap(*this, s);
	return *this;
}

StrokeHandler::Stroke::~Stroke() {
	if (annot != nullptr) {
		auto c = ctx->get_context();
		pdf_drop_annot(*c, annot);
	}
}

void StrokeHandler::apply_stroke_to_pdf(Stroke& s) {
	auto ctx = m_pdf->get_context();
	auto page = m_pdf->get_page(s.page); 
	auto rec = m_pdf->get_pagerec()->get_read()->at(s.page);
	s.annot = pdf_create_annot(*ctx, reinterpret_cast<pdf_page*>(*page), PDF_ANNOT_INK); 
	s.ctx = m_pdf->get_context_wrapper(); 
	std::array<fz_point, 4> temp_points;
	for (size_t i = 0; i < s.geometry.points.size() - 1; i++) {
		// transform the points to doc space
		temp_points[0] = s.geometry.points[i]            - rec.upperleft(); 
		temp_points[1] = s.geometry.control_points[0][i] - rec.upperleft();
		temp_points[2] = s.geometry.control_points[1][i] - rec.upperleft();
		temp_points[3] = s.geometry.points[i + 1]        - rec.upperleft();

		pdf_add_annot_ink_list(*ctx, s.annot, 4, &temp_points[0]);
	} 

	float b[] = { s.color.r/255.0f, s.color.g / 255.0f, s.color.b / 255.0f };
	pdf_set_annot_color(*ctx, s.annot, 3, b); 
}

std::vector<Renderer::Point<float>> get_points_from_annot(MuPDFHandler::PDF* pdf, pdf_annot* a) {
	std::vector<Renderer::Point<float>> all_points;
	auto ctx = pdf->get_context();
	auto list_count = pdf_annot_ink_list_count(*ctx, a);
	for (int i = 0; i < list_count; i++) {
		all_points.push_back(pdf_annot_ink_list_stroke_vertex(*ctx, a, i, 0)); 
	}
	return all_points; 
}

Renderer::Color get_color_from_annot(MuPDFHandler::PDF* pdf, pdf_annot* a) {
	auto ctx = pdf->get_context();
	int n = 0;
	float c[4];
	pdf_annot_color(*ctx, a, &n, c);
	// grey
	if (n == 1) {
		byte col = static_cast<byte>(c[0] * 255.0f);
		return Renderer::Color(col, col, col);
	}
	else if (n == 3) {
		return Renderer::Color(
			static_cast<byte>(c[0] * 255.0f),
			static_cast<byte>(c[1] * 255.0f),
			static_cast<byte>(c[2] * 255.0f)
		);
	}
	else {
		// TODO
		Logger::warn("Annot colorspace not supported");
	}
}

void StrokeHandler::parse_all_strokes() {
	auto ctx = m_pdf->get_context();
	auto page_amount = m_pdf->get_page_count();

	for (size_t i = 0; i < page_amount; i++) {
		auto current_page = m_pdf->get_page(i);
		
		auto annot = pdf_first_annot(*ctx, reinterpret_cast<pdf_page*>(*current_page)); 
		if (annot == nullptr) {
			continue;
		}
		auto type  = pdf_annot_type(*ctx, annot);
		do {
			type = pdf_annot_type(*ctx, annot);
			if (type == pdf_annot_type::PDF_ANNOT_INK) {
				auto has_ink = pdf_annot_has_ink_list(*ctx, annot);
				if (!has_ink) {
					Logger::warn("Found annotation without points... what?");
					continue;
				}
				// add it the the strokes as it may have to be deleted later on
				pdf_keep_annot(*ctx, annot);
				Stroke s;
				s.annot = annot;
				s.page = i;
				s.ctx = m_pdf->get_context_wrapper();
				s.points = get_points_from_annot(m_pdf, annot);
				s.thickness = pdf_annot_border_width(*ctx, annot);
				s.color = get_color_from_annot(m_pdf, annot);
				
				m_strokes.push_back(std::move(s));
			}
		} while ((annot = pdf_next_annot(*ctx, annot)) != NULL);
	}
	Logger::log("Found ", m_strokes.size(), " ink annotations in the document");
}

std::optional<size_t> StrokeHandler::get_page_from_point(Renderer::Point<float> p) {
	auto recs = m_pdf->get_pagerec()->get_read();
	for (size_t i = 0; i < recs->size(); i++) {
		if (recs->at(i).intersects(p)) {
			return i;
		}
	}
	return std::nullopt;
}

StrokeHandler::StrokeHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window) : m_pdf(pdf), m_renderer(renderer), m_window(window ){
	parse_all_strokes();
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
	//ASSERT(m_active_strokes.find(p.id) == m_active_strokes.end(), "Trying to start a new ink stroke with a pen that is already drawing a stroke");
	auto rec = m_pdf->get_pagerec()->get_read();

	auto page = get_page_from_point(m_renderer->inv_transform_point(p.pos));
	if (page == std::nullopt) {
		return;
	}

	// add a new strok to the map
	Stroke s;
	s.points.push_back(m_renderer->inv_transform_point(p.pos));
	s.page = page.value();

	m_active_strokes[p.id] = std::move(s);
}

void StrokeHandler::update_stroke(const WindowHandler::PointerInfo& p) {
	if (m_earising_points.size() != 0) {
		// we shouldn't be deleting strokes while creating new ones
		Logger::warn("Trying to create a new stroke while we are still earising points");
	}
	// First check if the stroke with the id already exists
	// and start a new one
	if (m_active_strokes.find(p.id) == m_active_strokes.end()) {
		start_stroke(p);
	}
	// Check if the annot is still on the page and end stroke if it isnt
	auto page = get_page_from_point(m_renderer->inv_transform_point(p.pos));
	if (page == std::nullopt || page.value() != m_active_strokes.at(p.id).page) { 
		end_stroke(p);
		return;
	}

	m_active_strokes[p.id].points.push_back(m_renderer->inv_transform_point(p.pos));
}

void StrokeHandler::end_stroke(const WindowHandler::PointerInfo& p) {
	if (m_earising_points.size() != 0) {
		// we shouldn't be deleting strokes while creating new ones
		Logger::warn("Trying to end a stroke while we are still earising points");
	}

	// First check if the stroke with the id already exists
	if (m_active_strokes.find(p.id) == m_active_strokes.end()) {
		return;
	}

	// update for the last time
	Stroke& s = m_active_strokes.at(p.id);
	s.points.push_back(m_renderer->inv_transform_point(p.pos));
	s.geometry = Renderer::create_bezier_geometry(m_active_strokes.at(p.id).points); // create the bezier geometry
	s.path = m_renderer->create_bezier_path(m_active_strokes.at(p.id).geometry);
	apply_stroke_to_pdf(s); // apply the stroke to the pdf document

	// add the stroke to the list of strokes
	m_strokes.push_back(std::move(s));
	m_active_strokes.erase(p.id);
}

void StrokeHandler::earsing_stroke(const WindowHandler::PointerInfo& p) {
	if (m_active_strokes.size() != 0) {
		// there should be no other active strokes
		Logger::warn("Trying to erase a stroke while there are still active strokes");
	}
	m_earising_points.push_back(m_renderer->inv_transform_point(p.pos)); 
	if (m_earising_points.size() > 10) { 
		m_earising_points.erase(m_earising_points.begin());
	}

	if (m_earising_points.size() >= 2) {
		// we can check if the last two added points overlap with any other curve
		auto p1 = m_earising_points.at(m_earising_points.size() - 2);
		auto p2 = m_earising_points.at(m_earising_points.size() - 1);
		// iterate through all strokes and check if the line intersects with any of them
		for (size_t i = 0; i < m_strokes.size(); i++) {
			auto& stroke = m_strokes.at(i);
			// special case where stroke.points.size == 1
			if (stroke.points.size() == 1 and Renderer::line_segment_intersects(p1, p2, stroke.points.at(0), stroke.points.at(0))) {
				m_index_of_earising_points.push_back(i);
				stroke.to_be_earesed = true;
				continue;
			}

			for (size_t j = 0; j < stroke.points.size() - 1; j++) {
				if (Renderer::line_segment_intersects(p1, p2, stroke.points.at(j), stroke.points.at(j + 1))) {
					m_index_of_earising_points.push_back(i);
					stroke.to_be_earesed = true;
					break;
				}
			}
		}
	}
	
}

void StrokeHandler::end_earsing_stroke(const WindowHandler::PointerInfo& p) {
	if (m_earising_points.size() == 0) {
		return;
	}
	earsing_stroke(p);
	// we now delete the strokes from the list
	std::sort(m_index_of_earising_points.begin(), m_index_of_earising_points.end(), std::greater<>());
	size_t last_index = ~0;
	for (size_t i = 0; i < m_index_of_earising_points.size(); i++) {
		auto index = m_index_of_earising_points.at(i);
		// in case of duplicates
		if (last_index == index) {
			continue;
		}
		last_index = index;
		auto ctx = m_strokes.at(index).ctx->get_context();
		auto page = m_pdf->get_page(m_strokes.at(index).page);
		pdf_delete_annot(*ctx, reinterpret_cast<pdf_page*>(*page), m_strokes.at(index).annot);

		m_strokes.erase(m_strokes.begin() + index);
	}
	m_index_of_earising_points.clear();
	m_earising_points.clear();
}

void StrokeHandler::render_strokes() {
	// iterate through all strokes and draw them using the renderer
	m_renderer->set_current_transform_active();
	m_renderer->begin_draw();
	for (size_t i = 0; i < m_strokes.size(); i++) {
		auto& stroke = m_strokes.at(i);
		// check if a path exist
		// A path object may not exist if the stroke was generated from the pdf 
		if (stroke.path.m_object == nullptr) {
			if (stroke.to_be_earesed) 
				for (size_t j = 0; j < stroke.points.size() - 1; j++) {
					m_renderer->draw_line(stroke.points.at(j), stroke.points.at(j + 1), {255, 0, 0}, stroke.thickness * 2);
					m_renderer->draw_line(stroke.points.at(j), stroke.points.at(j + 1), stroke.color, stroke.thickness);
				}
			continue;
		}

		if (stroke.to_be_earesed) {
			m_renderer->draw_path(stroke.path, {255, 0, 0}, stroke.thickness * 2);
		}
		m_renderer->draw_path(stroke.path, stroke.color, stroke.thickness); 
		
	}

	// also do it for the active ones
	for (auto& [id, stroke] : m_active_strokes) {
		for (size_t j = 0; j < stroke.points.size() - 1; j++) {
			m_renderer->draw_line(stroke.points.at(j), stroke.points.at(j + 1), stroke.color, stroke.thickness);
		}
	}

	// we can also render the earising points
	for (size_t i = 0; m_earising_points.size() > 0 and i < m_earising_points.size() - 1; i++) {
		m_renderer->draw_line(m_earising_points.at(i), m_earising_points.at(i + 1), {0, 0, 255}, 4);
		m_renderer->draw_line(m_earising_points.at(i), m_earising_points.at(i + 1), {0, 255, 255}, 1);
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