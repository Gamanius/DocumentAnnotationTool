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
template<typename T>
void remove_small_rects(std::vector <Renderer::Rectangle<T>>& rects, float threshold = EPSILON) {
	for (int i = rects.size() - 1; i >= 0; i--) {
		rects.at(i).validate();
		if (rects.at(i).width < threshold or rects.at(i).height < threshold) {
			rects.erase(rects.begin() + i);
		}
	}
}

void PDFHandler::render_high_res(HWND window) {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	std::vector<std::tuple<size_t, Renderer::Rectangle<float>>> clipped_documents;

	///////// ACCESS TO CACHED BITMAPS //////////
	std::lock_guard lock(m_cachedBitmaplock);

	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		// first check if they intersect
		if (m_pagerec.at(i).intersects(clip_space)) {
			// this is the overlap in the clip space
			auto overlap = clip_space.calculate_overlap(m_pagerec.at(i)); 

			// add all bitmaps sizes of the bitmaps with high enough dpi into an array
			std::vector<Renderer::Rectangle<float>> bitmap_dest;

			bitmap_dest.reserve(m_cachedBitmaps.size());
			for (auto& j : m_cachedBitmaps) {
				// check if the dpi is high enough
				if (FLOAT_EQUAL(j.dpi, dpi) == false)
					continue;

				bitmap_dest.push_back(j.dest);
			}

			// if there are no cached bitmaps we have to render everything
			if (bitmap_dest.empty()) {
				clipped_documents.push_back(std::make_tuple(i, overlap));
				continue;
			}

			// now we can chop up the render target. These are all the rectangles that we need to render
			std::vector<Renderer::Rectangle<float>> choppy = splice_rect(overlap, bitmap_dest);
			merge_rects(choppy); 
			remove_small_rects(choppy);
			Logger::log("Choppy size: " + std::to_string(choppy.size()) + "\n");
			Logger::print_to_debug();

			/*
			overlap.x -= m_pagerec.at(i).x;
			overlap.y -= m_pagerec.at(i).y;
			*/
			for (size_t j = 0; j < choppy.size(); j++) {
				// just in case
				choppy.at(j).validate();
				// transform into doc space
				choppy.at(j).x -= m_pagerec.at(i).x;
				choppy.at(j).y -= m_pagerec.at(i).y;

				// make choppy bigger so the are no stitch lines
				choppy.at(j).x -= 2;  /* this is just a magic number */
				choppy.at(j).y -= 2; 
				choppy.at(j).width += 4;
				choppy.at(j).height += 4;

				// and push back
				clipped_documents.push_back(std::make_tuple(i, choppy.at(j)));
			}

			// add it to the pile
			//clipped_documents.push_back(std::make_tuple(i, overlap));
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
		//m_renderer->begin_draw(); 
		
		//m_renderer->draw_bitmap(butmap, overlap, Direct2DRenderer::INTERPOLATION_MODE::LINEAR); 
		//m_renderer->end_draw();
	}
	SendMessage(window, WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&m_cachedBitmaps));

	remove_small_cached_bitmaps(50 / m_renderer->get_transform_scale());
	while (m_cachedBitmaps.size() > 50) {
		m_cachedBitmaps.pop_front(); 
	}

	////run through all documents and create threads
	//for (size_t i = 0; i < clipped_documents.size(); i++) {
	//	auto page        = std::get<0>(clipped_documents.at(i));
	//	auto overlap     = std::get<1>(clipped_documents.at(i));
	//	auto dest		 = overlap;
	//
	//	dest.x += m_pagerec.at(page).x;
	//	dest.y += m_pagerec.at(page).y;
	//
	//	m_pdf.multithreaded_get_bitmap(m_renderer, page, overlap, dest, dpi); 
	//
	//}
}

void PDFHandler::remove_small_cached_bitmaps(float treshold) {
	for (int i = m_cachedBitmaps.size() - 1; i >= 0; i--) {
		if (m_cachedBitmaps.at(i).dest.width < treshold or m_cachedBitmaps.at(i).dest.height < treshold) {
			m_cachedBitmaps.erase(m_cachedBitmaps.begin() + i);
		}
	}
}

void PDFHandler::debug_render_preview() {
	// check for intersection. if it intersects than do the rendering
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		if (m_pagerec.at(i).intersects(clip_space)) {
			m_renderer->draw_rect_filled(m_pagerec.at(i), { 255, 255, 255});
		}
	}
	m_renderer->end_draw();
}

void PDFHandler::debug_render_high_res() {
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();

	Renderer::Rectangle<float> clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());

	std::vector<std::tuple<size_t, Renderer::Rectangle<float>>> clipped_documents;
	
	std::vector<Renderer::Rectangle<float>> cached_rects;

	for (size_t i = 0; i < m_pdf.get_page_count(); i++) { 
		if (m_pagerec.at(i).intersects(clip_space)) { 
			auto overlap = clip_space.validate().calculate_overlap(m_pagerec.at(i));

			std::vector<Renderer::Rectangle<float>> bitmap_dest;
			for (auto& j : m_debug_cachedBitmap) {
				// check if the dpi is high enough
				if (FLOAT_EQUAL(std::get<1>(j)/*dpi*/, dpi) == false)
					continue;

				bitmap_dest.push_back(std::get<0>(j));
				cached_rects.push_back(std::get<0>(j));
			}

			if (bitmap_dest.empty()) {
				clipped_documents.push_back(std::make_tuple(i, overlap));
				continue;
			}

			//merge_rects(bitmap_dest); 

			std::vector<Renderer::Rectangle<float>> choppy = splice_rect(overlap, bitmap_dest);
			merge_rects(choppy);

			for (size_t j = 0; j < choppy.size(); j++) {
				// just in case
				choppy.at(j).validate();
				// we can remove any rectangle that is too small
				if (choppy.at(j).width < 0.01f or choppy.at(j).height < 0.01f)
					continue;
				// transform into doc space
				choppy.at(j).x -= m_pagerec.at(i).x;
				choppy.at(j).y -= m_pagerec.at(i).y;

				// and push back
				clipped_documents.push_back(std::make_tuple(i, choppy.at(j)));
			}
		}
	}

	for (size_t i = 0; i < clipped_documents.size(); i++) {
		auto page = std::get<0>(clipped_documents.at(i));
		auto overlap = std::get<1>(clipped_documents.at(i));
		
		overlap.x += m_pagerec.at(page).x;
		overlap.y += m_pagerec.at(page).y;
		m_renderer->draw_rect(overlap, { 255, 0 ,0 }, 10);

		m_debug_cachedBitmap.push_back(std::make_tuple(overlap, dpi));
	}

	for (size_t i = 0; i < cached_rects.size(); i++) {
		auto r = cached_rects.at(i);

		m_renderer->draw_rect(r, {0, 255, 0}, 5);
	}

	merge_rects(cached_rects);
	for (size_t i = 0; i < cached_rects.size(); i++) {
		auto r = cached_rects.at(i);

		m_renderer->draw_rect(r, { 0, 0, 255 }, 5);
	}

	for (int i = m_debug_cachedBitmap.size() - 1; i >= 0; i--) {
		if (std::get<0>(m_debug_cachedBitmap.at(i)).width < 50 or std::get<0>(m_debug_cachedBitmap.at(i)).height < 50) {
			m_debug_cachedBitmap.erase(m_debug_cachedBitmap.begin() + i);
		}
	}

	while (m_debug_cachedBitmap.size() > 20) {
		m_debug_cachedBitmap.pop_front();
	}
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

void PDFHandler::debug_render() {
	auto scale = m_renderer->get_transform_scale();

	debug_render_preview();
	if (scale > m_preview_scale + EPSILON) {
		debug_render_high_res();
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
