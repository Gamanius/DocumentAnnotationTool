#include "include.h"


Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi) const {
	fz_matrix ctm;
	auto size = m_pdf->get_page_size(page, dpi);
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	fz_try(*m_pdf) {
		pixmap = fz_new_pixmap(*m_pdf, fz_device_rgb(*m_pdf), size.width, size.height, nullptr, 1);
		fz_clear_pixmap_with_value(*m_pdf, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*m_pdf, fz_identity, pixmap);
		fz_run_page(*m_pdf, m_pdf->get_page(page), drawdevice, ctm, nullptr);
		fz_close_device(*m_pdf, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
	} fz_always(*m_pdf) {
		fz_drop_device(*m_pdf, drawdevice);
		fz_drop_pixmap(*m_pdf, pixmap);
	} fz_catch(*m_pdf) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj);
}

Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<unsigned int> rec) const {
	fz_matrix ctm;
	auto size = m_pdf->get_page_size(page); 
	ctm = fz_scale((rec.width / size.width), (rec.height / size.height));

	// this is to calculate the size of the pixmap
	auto bbox = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, size.width, size.height), ctm));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	fz_try(*m_pdf) {
		pixmap = fz_new_pixmap_with_bbox(*m_pdf, fz_device_rgb(*m_pdf), bbox, nullptr, 1);
		fz_clear_pixmap_with_value(*m_pdf, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*m_pdf, fz_identity, pixmap);
		fz_run_page(*m_pdf, m_pdf->get_page(page), drawdevice, ctm, nullptr);
		fz_close_device(*m_pdf, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, rec, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
	} fz_always(*m_pdf) {
		fz_drop_device(*m_pdf, drawdevice);
		fz_drop_pixmap(*m_pdf, pixmap);
	} fz_catch(*m_pdf) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj);
}

Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, Renderer::Rectangle<float> source, float dpi) const {
	fz_matrix ctm;
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));
	auto transform = fz_translate(-source.x, -source.y);
	// this is to calculate the size of the pixmap using the source
	auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, source.width, source.height), ctm));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	fz_try(*m_pdf) {
		pixmap = fz_new_pixmap_with_bbox(*m_pdf, fz_device_rgb(*m_pdf), pixmap_size, nullptr, 1);
		fz_clear_pixmap_with_value(*m_pdf, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*m_pdf, fz_identity, pixmap);
		fz_run_page(*m_pdf, m_pdf->get_page(page), drawdevice, fz_concat(transform, ctm), nullptr);
		fz_close_device(*m_pdf, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, dpi); // default dpi of the pixmap
	} fz_always(*m_pdf) {
		fz_drop_device(*m_pdf, drawdevice);
		fz_drop_pixmap(*m_pdf, pixmap);
	} fz_catch(*m_pdf) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj);
}

void PDFRenderHandler::create_display_list() {
	// for each rec create a display list
	fz_display_list* list = nullptr;
	fz_device* dev = nullptr; 
		
	m_display_lists = std::make_shared<std::vector<fz_display_list*>>();

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		// get the page that will be rendered
		auto p = m_pdf->get_page(i);
		fz_try(*m_pdf) {
			// create a display list with all the draw calls and so on
			list = fz_new_display_list(*m_pdf, fz_bound_page(*m_pdf, p));
			dev = fz_new_list_device(*m_pdf, list);
			// run the device
			fz_run_page(*m_pdf, p, dev, fz_identity, nullptr);
			// add list to array
			m_display_lists->push_back(list);
		} fz_always(*m_pdf) {
			// flush the device
			fz_close_device(*m_pdf, dev);
			fz_drop_device(*m_pdf, dev);
		} fz_catch(*m_pdf) {
			ASSERT(false, "Could not create display list");
		}
	}
}

typedef struct {
	unsigned int cmd : 5;
	unsigned int size : 9;
	unsigned int rect : 1;
	unsigned int path : 1;
	unsigned int cs : 3;
	unsigned int color : 1;
	unsigned int alpha : 2;
	unsigned int ctm : 3;
	unsigned int stroke : 1;
	unsigned int flags : 6;
} fz_display_node;


struct fz_display_list {
	fz_storable storable;
	fz_display_node* list;
	fz_rect mediabox;
	size_t max;
	size_t len;
};

PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer) : m_pdf(pdf), m_renderer(renderer) {
	// init of m_pagerec
	sort_page_positions();
	// init of m_previewBitmaps
	create_preview(1); // create preview with default scale

	m_cachedBitmaps = std::make_shared<std::deque<CachedBitmap>>();

	create_display_list();
}

PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, size_t amount_of_render) : PDFRenderHandler(pdf, renderer) {
	//create_render_threads(amount_of_render);
}

void PDFRenderHandler::create_preview(float scale) {
	m_previewBitmaps.clear();

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		m_previewBitmaps.push_back(get_bitmap(*m_renderer, i, m_renderer->get_dpi() * scale));
	}

	m_preview_scale = scale;
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

std::vector<size_t> PDFRenderHandler::render_job_clipped_cashed_bitmaps(Renderer::Rectangle<float> overlap_clip_space, float dpi) {
	std::vector<size_t> return_value;
	for (size_t i = 0; i < m_cachedBitmaps->size(); i++) {
		// check if the dpi is high enough
		if (m_cachedBitmaps->at(i).dpi < dpi and FLOAT_EQUAL(m_cachedBitmaps->at(i).dpi, dpi) == false)
			continue;
		if (overlap_clip_space.intersects(m_cachedBitmaps->at(i).dest) == false)
			continue;

		return_value.push_back(i);
	}
	return return_value;
}

std::vector<Renderer::Rectangle<float>> PDFRenderHandler::render_job_splice_recs(Renderer::Rectangle<float> overlap, const std::vector<size_t>& cashedBitmapindex) {
	std::vector<Renderer::Rectangle<float>> other;
	other.reserve(cashedBitmapindex.size());
	for (size_t i = 0; i < cashedBitmapindex.size(); i++) {
		other.push_back(m_cachedBitmaps->at(cashedBitmapindex.at(i)).dest);
	}

	std::vector<Renderer::Rectangle<float>> choppy = splice_rect(overlap, other);
	merge_rects(choppy);
	remove_small_rects(choppy);
	
	for (size_t i = 0; i < choppy.size(); i++) {
		choppy.at(i).validate();
	}
	 
	return choppy; 
} 

void PDFRenderHandler::render_high_res(HWND window) {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());

	std::vector<std::tuple<size_t, Renderer::Rectangle<float>, std::vector<CachedBitmap*>>> clipped_documents;

	std::vector<RenderInfo> clipped_render_queue;


	reduce_cache_size(500);
	///////// ACCESS TO CACHED BITMAPS //////////

	// TODO
	//update_render_queue();

	auto lambda_add_stitch = [](Renderer::Rectangle<float>& r) {
			r.x -= PDF_STITCH_THRESHOLD;
			r.y -= PDF_STITCH_THRESHOLD;
			r.width += PDF_STITCH_THRESHOLD * 2;
			r.height += PDF_STITCH_THRESHOLD * 2;
		};
	
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		// first check if they intersect
		if (m_pagerec.at(i).intersects(clip_space)) {
			// this is the overlap in the clip space
			auto overlap = clip_space.calculate_overlap(m_pagerec.at(i)); 

			//Check if page overlaps with any chached bitmap
			std::vector<size_t> cached_bitmap_index = render_job_clipped_cashed_bitmaps(overlap, dpi);

			// if there are no cached bitmaps we have to render everything
			if (cached_bitmap_index.empty()) {
				// add stitch
				lambda_add_stitch(overlap);

				// need to transfom into doc space
				overlap.x -= m_pagerec.at(i).x;
				overlap.y -= m_pagerec.at(i).y;

				// add to the queue
				clipped_render_queue.push_back({i, overlap});

				continue;
			}

			// now we can chop up the render target. These are all the rectangles that we need to render
			auto chopped_render_targets = render_job_splice_recs(overlap, cached_bitmap_index);
			
			for (size_t j = 0; j < chopped_render_targets.size(); j++) {
				// transform into doc space
				chopped_render_targets.at(j).x -= m_pagerec.at(i).x;
				chopped_render_targets.at(j).y -= m_pagerec.at(i).y;

				// make choppy bigger so the are no stitch lines
				lambda_add_stitch(chopped_render_targets.at(j));

				// add the chopped up recs to the queue
				clipped_render_queue.push_back({ i, chopped_render_targets.at(j) });
			}

			// add all bitmaps into an array so it can be send to the main window
			std::vector<CachedBitmap*> temp_cachedBitmaps;
			for (size_t i = 0; i < cached_bitmap_index.size(); i++) {
				temp_cachedBitmaps.push_back(&m_cachedBitmaps->at(cached_bitmap_index.at(i)));
			}
			// we can send some of the cached bitmaps already to the window
			SendMessage(window, WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
		}
	}

	std::vector<CachedBitmap*> temp_cachedBitmaps; 
	// NON multithreaded solution
	for (size_t i = 0; i < clipped_render_queue.size(); i++) {
		auto page = clipped_render_queue.at(i).page;
		auto overlap = clipped_render_queue.at(i).overlap_in_docspace;
		auto butmap = get_bitmap(*m_renderer, page, overlap, dpi); 

		// transform from doc space to clip space
		overlap.x += m_pagerec.at(page).x;
		overlap.y += m_pagerec.at(page).y;

		// add all bitmaps to the cache
		m_cachedBitmaps->push_back({ std::move(butmap), overlap, dpi, 0 });
		// add to an temporary array so it can be send to the main window
		temp_cachedBitmaps.push_back(&m_cachedBitmaps->back());

	}

	// send the new rendered bitmaps to the window
	SendMessage(window, WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
}

void PDFRenderHandler::remove_small_cached_bitmaps(float treshold) {
	if (m_cachedBitmaps->size() == 0)
		return;

	for (int i = m_cachedBitmaps->size() - 1; i >= 0; i--) {
		if ((m_cachedBitmaps->at(i).dest.width < treshold or m_cachedBitmaps->at(i).dest.height < treshold) and m_cachedBitmaps->at(i).used == 0) {
			m_cachedBitmaps->erase(m_cachedBitmaps->begin() + i);
		}
	}
}

void PDFRenderHandler::reduce_cache_size(unsigned long long max_memory) {
	remove_small_cached_bitmaps(50 / m_renderer->get_transform_scale()); 
	auto mem_usage = get_cache_memory_usage()/1000000;
	
	while (mem_usage > max_memory) {
		m_cachedBitmaps->pop_front(); 
		mem_usage = get_cache_memory_usage() / 1000000;
	}
}

void PDFRenderHandler::debug_render_preview() {
	// check for intersection. if it intersects than do the rendering
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		if (m_pagerec.at(i).intersects(clip_space)) {
			m_renderer->draw_rect_filled(m_pagerec.at(i), { 255, 255, 255});
		}
	}
	m_renderer->end_draw();
}

void PDFRenderHandler::debug_render_high_res() {
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();

	Renderer::Rectangle<float> clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());

	std::vector<std::tuple<size_t, Renderer::Rectangle<float>>> clipped_documents;
	
	std::vector<Renderer::Rectangle<float>> cached_rects;

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) { 
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

PDFRenderHandler::PDFRenderHandler(PDFRenderHandler&& t) noexcept : m_renderer(t.m_renderer), m_pdf(t.m_pdf) {
	m_previewBitmaps = std::move(t.m_previewBitmaps); 

	m_preview_scale = t.m_preview_scale;
	t.m_preview_scale = 0;

	m_seperation_distance = t.m_seperation_distance;
	t.m_seperation_distance = 0;

	// High res rendering
	m_display_lists = std::move(t.m_display_lists);
	m_cachedBitmaps = std::move(t.m_cachedBitmaps);

	m_pagerec = std::move(t.m_pagerec);
}

PDFRenderHandler& PDFRenderHandler::operator=(PDFRenderHandler&& t) noexcept {
	// new c++ stuff?
	this->~PDFRenderHandler();
	new(this) PDFRenderHandler(std::move(t)); 
	return *this;
}

void PDFRenderHandler::render_preview() {
	// check for intersection. if it intersects than do the rendering
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		if (m_pagerec.at(i).intersects(clip_space)) {
			m_renderer->draw_bitmap(m_previewBitmaps.at(i), m_pagerec.at(i), Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
		}
	}
	m_renderer->end_draw();
}

void PDFRenderHandler::render(HWND callbackwindow) {
	auto scale = m_renderer->get_transform_scale();
	//render_preview();

	render_preview();
	if (scale > m_preview_scale + EPSILON) {
		// if we are zoomed in, we need to render the page at a higher resolution
		// than the screen resolution
		render_high_res(callbackwindow);
	}
}


//void PDFRenderHandler::stop_rendering(HWND callbackwindow) {
//	m_pdf.stop_render_threads(false);
//	//first block
//	MSG msg;
//	while (m_pdf.multithreaded_is_rendering_in_progress())
//		GetMessage(&msg, 0, 0, 0);
//}

void PDFRenderHandler::debug_render() {
	auto scale = m_renderer->get_transform_scale();

	debug_render_preview();
	if (scale > m_preview_scale + EPSILON) {
		debug_render_high_res();
	}
}

void PDFRenderHandler::sort_page_positions() {
	m_pagerec.clear();
	double height = 0;
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		auto size = m_pdf->get_page_size(i);
		m_pagerec.push_back(Renderer::Rectangle<double>(0, height, size.width, size.height));
		height += m_pdf->get_page_size(i).height; 
		height += m_seperation_distance; 
	}
}

unsigned long long PDFRenderHandler::get_cache_memory_usage() const {
	unsigned long long mem = 0;
	for (size_t i = 0; i < m_cachedBitmaps->size(); i++) {
		auto d = m_cachedBitmaps->at(i).bitmap.m_object->GetPixelSize();
		mem += static_cast<unsigned long long>(d.height) * d.width * 4;
	}
	return mem;
}

PDFRenderHandler::~PDFRenderHandler() {
	if (m_display_lists == nullptr)
		return;
	for (size_t i = 0; i < m_display_lists->size(); i++) {
		fz_drop_display_list(m_pdf->get_context(), m_display_lists->at(i));
	}
}
