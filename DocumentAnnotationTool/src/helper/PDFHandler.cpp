#include "include.h"

void PDFHandler::create_preview(float scale) {
	m_previewBitmaps.clear();

	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		m_previewBitmaps.push_back(m_pdf.get_bitmap(*m_renderer, i, m_renderer->get_dpi() * scale));
	}

	m_preview_scale = scale;
}

void multithreaded_high_res(Direct2DRenderer::BitmapObject obj, Direct2DRenderer* renderer, PDFHandler* handler) {

}

void PDFHandler::render_high_res(HWND window) {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	std::vector<std::tuple<size_t, Renderer::Rectangle<float>>> clipped_documents;
	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		// first check if they intersect
		if (m_pagerec.at(i).intersects(clip_space)) {
			// this is the overlap in the clip space
			auto overlap = clip_space.calculate_overlap(m_pagerec.at(i)); 
			// we have to check if the overlap has already been rendered before
			for (auto& i : m_cachedBitmaps) {
				// check if the dpi is high enough
				if (i.dpi < dpi - EPSILON)
					continue;
				// first check if they intersect
				if (overlap.intersects(i.dest) == false)
					continue;
				//https://www.desmos.com/geometry/esxorzxlk9

				// if the intersect we have to calculate the new rectangle in the clip space
				auto cached_overlap = overlap.calculate_overlap(i.dest); 
				Renderer::Rectangle<float> r1 = { min(cached_overlap.x, i.dest.x), min(cached_overlap.y, i.dest.y), 
					max(cached_overlap.x + cached_overlap.width, i.dest.x + i.dest.width), max(cached_overlap.y + cached_overlap.height, i.dest.y + i.dest.height) };
			}
			// transform into doc space
			overlap.x -= m_pagerec.at(i).x;
			overlap.y -= m_pagerec.at(i).y;
			// add it to the pile
			clipped_documents.push_back(std::make_tuple(i, overlap));
		}
	}

	// NON multithreaded solution
	for (size_t i = 0; i < clipped_documents.size(); i++) {
		auto page = std::get<0>(clipped_documents.at(i));
		auto overlap = std::get<1>(clipped_documents.at(i));
		auto butmap = m_pdf.get_bitmap(*m_renderer, page, overlap, dpi);
		// transform from doc space to clip space
		overlap.x += m_pagerec.at(page).x;
		overlap.y += m_pagerec.at(page).y;
		m_cachedBitmaps.push_back({std::move(butmap), overlap, dpi });
		SendMessage(window, WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&m_cachedBitmaps));
		//m_renderer->begin_draw(); 
		
		//m_renderer->draw_bitmap(butmap, overlap, Direct2DRenderer::INTERPOLATION_MODE::LINEAR); 
		//m_renderer->end_draw();
	}

	//run through all documents and create threads
	//for (size_t i = 0; i < clipped_documents.size(); i++) {
	//	// divide rec into 4 equal parts
	//	auto page        = std::get<0>(clipped_documents.at(i));
	//	// in document space
	//	auto overlap     = std::get<1>(clipped_documents.at(i));
	//	// in clip space
	//	auto dest		 = overlap;
	//
	//	dest.x += m_pagerec.at(page).x;
	//	dest.y += m_pagerec.at(page).y;
	//
	//	m_pdf.multithreaded_get_bitmap(m_renderer, page, overlap, dest, m_renderer->get_dpi() * m_renderer->get_transform_scale(), this);
	//
	//}
}

PDFHandler::PDFHandler(Direct2DRenderer* const renderer, MuPDFHandler& handler, const std::wstring& path) : m_renderer(renderer) {
	// will raise error if path is invalid so we dont need to do assert here
	auto d = handler.load_pdf(path);
	if (!d.has_value()) {
		return;
	}
	m_pdf = std::move(d.value());

	// init of m_pagerec
	sort_page_positions();
	// init of m_previewBitmaps
	create_preview(1); // create preview with default scale
}

PDFHandler::PDFHandler(PDFHandler&& t) noexcept : m_renderer(t.m_renderer) {
	m_pdf = std::move(t.m_pdf); 
	m_previewBitmaps = std::move(t.m_previewBitmaps); 

	m_preview_scale = t.m_preview_scale;
	t.m_preview_scale = 0;

	m_seperation_distance = t.m_seperation_distance;
	t.m_seperation_distance = 0;

	m_pagerec = std::move(t.m_pagerec);
}

PDFHandler& PDFHandler::operator=(PDFHandler&& t) noexcept {
	// new c++ stuff?
	this->~PDFHandler();
	new(this) PDFHandler(std::move(t)); 
	return *this;
}

void PDFHandler::render_preview() {
	// check for intersection. if it intersects than do the rendering
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		if (m_pagerec.at(i).intersects(clip_space)) {
			m_renderer->draw_bitmap(m_previewBitmaps.at(i), m_pagerec.at(i), Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
		}
	}
	m_renderer->end_draw();
}

void PDFHandler::render(HWND callbackwindow) {
	auto scale = m_renderer->get_transform_scale();
	//render_preview();

	render_preview();
	if (scale > m_preview_scale + EPSILON) {
		// if we are zoomed in, we need to render the page at a higher resolution
		// than the screen resolution
		render_high_res(callbackwindow);
	}
}

void PDFHandler::sort_page_positions() {
	m_pagerec.clear();
	double height = 0;
	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		auto size = m_pdf.get_page_size(i);
		m_pagerec.push_back(Renderer::Rectangle<double>(0, height, size.width, size.height));
		height += m_pdf.get_page_size(i).height; 
		height += m_seperation_distance; 
	}
}

void PDFHandler::add_cachedBitmap(Direct2DRenderer::BitmapObject bitmap, Renderer::Rectangle<float> dest, float dpi) {
	std::unique_lock lock(m_cachedBitmaplock);
	m_cachedBitmaps.push_back({ std::move(bitmap), dest, dpi });
}


PDFHandler::~PDFHandler() {
	//m_pdf will be destroyed automatically
	//m_preview will be destroyed automatically
}
