#include "include.h"

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

Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi) const {
	fz_matrix ctm;
	auto size = m_pdf->get_page_size(page, dpi);
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	auto ctx = m_pdf->get_context();
	auto pag = m_pdf->get_page(page);

	fz_try(*ctx) {
		pixmap = fz_new_pixmap(*ctx, fz_device_rgb(*ctx), size.width, size.height, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, *pag, drawdevice, ctm, nullptr);
		fz_close_device(*ctx, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
	} fz_always(*ctx) {
		fz_drop_device(*ctx, drawdevice);
		fz_drop_pixmap(*ctx, pixmap);
	} fz_catch(*ctx) {
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

	auto ctx = m_pdf->get_context();
	auto pag = m_pdf->get_page(page);

	fz_try(*ctx) {
		pixmap = fz_new_pixmap_with_bbox(*ctx, fz_device_rgb(*ctx), bbox, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, *pag, drawdevice, ctm, nullptr);
		fz_close_device(*ctx, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, rec, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
	} fz_always(*ctx) {
		fz_drop_device(*ctx, drawdevice);
		fz_drop_pixmap(*ctx, pixmap);
	} fz_catch(*ctx) {
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

	auto ctx = m_pdf->get_context();
	auto pag = m_pdf->get_page(page);

	fz_try(*ctx) {
		pixmap = fz_new_pixmap_with_bbox(*ctx, fz_device_rgb(*ctx), pixmap_size, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, *pag, drawdevice, fz_concat(transform, ctm), nullptr);
		fz_close_device(*ctx, drawdevice);
		obj = renderer.create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, dpi); // default dpi of the pixmap
	} fz_always(*ctx) {
		fz_drop_device(*ctx, drawdevice);
		fz_drop_pixmap(*ctx, pixmap);
	} fz_catch(*ctx) {
		return Direct2DRenderer::BitmapObject();
	}

	return std::move(obj);
}

void PDFRenderHandler::create_display_list() {
	// for each rec create a display list
	fz_display_list* list = nullptr;
	fz_device* dev = nullptr; 

	auto ctx = m_pdf->get_context();
	auto display_list = m_display_list->get_item();

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		// get the page that will be rendered
		auto p = m_pdf->get_page(i);
		fz_try(*ctx) {
			// create a display list with all the draw calls and so on
			list = fz_new_display_list(*ctx, fz_bound_page(*ctx, *p));
			dev = fz_new_list_device(*ctx, list);
			// run the device
			fz_run_page(*ctx, *p, dev, fz_identity, nullptr);
			// add list to array
			display_list->push_back(std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list)));
		} fz_always(*ctx) {
			// flush the device
			fz_close_device(*ctx, dev);
			fz_drop_device(*ctx, dev);
		} fz_catch(*ctx) {
			ASSERT(false, "Could not create display list");
		}
	}
}

void PDFRenderHandler::create_display_list_async() {
	// for each rec create a display list
	fz_display_list* list = nullptr; 
	fz_device* dev = nullptr; 

	// copy the context
	fz_context* copy_ctx = nullptr;
	{
		auto ctx = m_pdf->get_context();
		copy_ctx = fz_clone_context(*ctx);
	}
	
	// aquire the display list
	auto display_list = m_display_list->get_item();

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) { 
		// get the page that will be rendered
		auto p = m_pdf->get_page(i);
		fz_try(copy_ctx) {
			// create a display list with all the draw calls and so on
			list = fz_new_display_list(copy_ctx, fz_bound_page(copy_ctx, *p));
			dev = fz_new_list_device(copy_ctx, list);
			// run the device
			fz_run_page(copy_ctx, *p, dev, fz_identity, nullptr);
			// add list to array
			display_list->push_back(std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list)));
		} fz_always(copy_ctx) {
			// flush the device
			fz_close_device(copy_ctx, dev);
			fz_drop_device(copy_ctx, dev);
		} fz_catch(copy_ctx) {
			ASSERT(false, "Could not create display list"); 
		}
	}

	fz_drop_context(copy_ctx);
}


PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window) : m_pdf(pdf), m_renderer(renderer), m_window(window) {
	// create member object
	m_display_list = std::shared_ptr<ThreadSafeVector<std::shared_ptr<DisplayListWrapper>>>
		(new ThreadSafeVector<std::shared_ptr<DisplayListWrapper>>(std::vector<std::shared_ptr<DisplayListWrapper>>()));

	m_preview_bitmaps = std::shared_ptr<ThreadSafeVector<Direct2DRenderer::BitmapObject>>
		(new ThreadSafeVector<Direct2DRenderer::BitmapObject>(std::vector<Direct2DRenderer::BitmapObject>()));
	
	m_cachedBitmaps = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>())); 

	// init of m_pagerec
	sort_page_positions();
	// init of m_previewBitmaps
	//create_preview(1); // create preview with default scale


	m_display_list_thread = std::thread([this] { create_display_list_async(); });
	m_preview_bitmap_thread = std::thread([this] { create_preview(1); });
}


void PDFRenderHandler::create_preview(float scale) {
	auto prev = m_preview_bitmaps->get_item();
	prev->clear();

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		prev->push_back(get_bitmap(*m_renderer, i, m_renderer->get_dpi() * scale));
		{
			// add to the cached bitmaps
			auto cached = m_cachedBitmaps->get_item();
			CachedBitmap p;
			p.bitmap = prev->back();
			p.dest = m_pagerec[i];
			cached->push_back(std::move(p)); 
		}
		m_window->invalidate_drawing_area(); 
	}

	m_preview_scale = scale;
	m_preview_bitmaps_processed = true;
	// send window signal
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
	auto cached_bitmap = m_cachedBitmaps->get_item();
	std::vector<size_t> return_value;
	for (size_t i = 0; i < cached_bitmap->size(); i++) {
		// check if the dpi is high enough
		if (cached_bitmap->at(i).dpi < dpi and FLOAT_EQUAL(cached_bitmap->at(i).dpi, dpi) == false)
			continue;
		if (overlap_clip_space.intersects(cached_bitmap->at(i).dest) == false)
			continue;

		return_value.push_back(i);
	}
	return return_value;
}

std::vector<Renderer::Rectangle<float>> PDFRenderHandler::render_job_splice_recs(Renderer::Rectangle<float> overlap, const std::vector<size_t>& cashedBitmapindex) {
	std::vector<Renderer::Rectangle<float>> other;	
	auto cached_bitmaps = m_cachedBitmaps->get_item();

	other.reserve(cashedBitmapindex.size());
	for (size_t i = 0; i < cashedBitmapindex.size(); i++) {
		other.push_back(cached_bitmaps->at(cashedBitmapindex.at(i)).dest);
	}

	std::vector<Renderer::Rectangle<float>> choppy = splice_rect(overlap, other);
	merge_rects(choppy);
	remove_small_rects(choppy);
	
	for (size_t i = 0; i < choppy.size(); i++) {
		choppy.at(i).validate();
	}
	 
	return choppy; 
} 

void PDFRenderHandler::render_high_res() {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());

	std::vector<std::tuple<size_t, Renderer::Rectangle<float>, std::vector<CachedBitmap*>>> clipped_documents;

	std::vector<RenderInfo> clipped_render_queue;

	reduce_cache_size(500);
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
			auto cached_bitmaps = m_cachedBitmaps->get_item();
			for (size_t i = 0; i < cached_bitmap_index.size(); i++) {
				temp_cachedBitmaps.push_back(&cached_bitmaps->at(cached_bitmap_index.at(i)));
			}
			// we can send some of the cached bitmaps already to the window
			SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
		}
	}

	auto cached_bitmaps = m_cachedBitmaps->get_item();
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
		cached_bitmaps->push_back({ std::move(butmap), overlap, dpi, 0 });
		// add to an temporary array so it can be send to the main window
		temp_cachedBitmaps.push_back(&cached_bitmaps->back());

	}

	// send the new rendered bitmaps to the window
	SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
}

void PDFRenderHandler::remove_small_cached_bitmaps(float treshold) {
	auto cached_bitmaps = m_cachedBitmaps->get_item();
	if (cached_bitmaps->size() == 0)
		return;

	for (int i = cached_bitmaps->size() - 1; i >= 0; i--) {
		if ((cached_bitmaps->at(i).dest.width < treshold or cached_bitmaps->at(i).dest.height < treshold) and cached_bitmaps->at(i).used == 0) {
			cached_bitmaps->erase(cached_bitmaps->begin() + i);
		}
	}
}

void PDFRenderHandler::reduce_cache_size(unsigned long long max_memory) {
	auto cached_bitmaps = m_cachedBitmaps->get_item();

	//remove_small_cached_bitmaps(50 / m_renderer->get_transform_scale()); 
	auto mem_usage = get_cache_memory_usage()/1000000;
	
	while (mem_usage > max_memory) {
		cached_bitmaps->pop_front();
		mem_usage = get_cache_memory_usage() / 1000000;
	}
}

PDFRenderHandler::PDFRenderHandler(PDFRenderHandler&& t) noexcept : m_renderer(t.m_renderer), m_pdf(t.m_pdf), m_window(t.m_window) {
	m_preview_bitmaps = std::move(t.m_preview_bitmaps); 
	m_preview_bitmap_thread = std::move(t.m_preview_bitmap_thread);

	m_preview_scale = t.m_preview_scale;
	t.m_preview_scale = 0;

	m_seperation_distance = t.m_seperation_distance;
	t.m_seperation_distance = 0;

	// High res rendering
	m_display_list = std::move(t.m_display_list);
	m_display_list_thread = std::move(t.m_display_list_thread);
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
	auto prev = m_preview_bitmaps->get_item();
	// check for intersection. if it intersects than do the rendering
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		if (m_pagerec.at(i).intersects(clip_space)) {
			m_renderer->draw_bitmap(prev->at(i), m_pagerec.at(i), Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
		}
	}
	m_renderer->end_draw();
}

void PDFRenderHandler::render_outline() {
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size()); 

	for (size_t i = 0; i < m_pagerec.size(); i++) {
		if (m_pagerec.at(i).intersects(clip_space)) {
			m_renderer->draw_rect_filled(m_pagerec.at(i), { 255, 255, 255 });
		}
	}

	// draww all cached bitmaps
	std::vector<CachedBitmap*> temp;
	{
		// access to cached bitmaps
		auto cached = m_cachedBitmaps->get_item();
		for (size_t i = 0; i < cached->size(); i++) {
			if (cached->at(i).dest.intersects(clip_space)) {
				temp.push_back(&cached->at(i));
			}
		}

		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp));
	}

	m_renderer->end_draw();
}

void PDFRenderHandler::render() {
	if (m_preview_bitmaps_processed == false) {
		render_outline();
		return;
	}
	auto scale = m_renderer->get_transform_scale();
	//render_preview();

	render_preview();
	if (scale > m_preview_scale + EPSILON) {
		// if we are zoomed in, we need to render the page at a higher resolution
		// than the screen resolution
		render_high_res();
	}
}

void PDFRenderHandler::debug_render() {
	auto scale = m_renderer->get_transform_scale();
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
	auto cached_bitmaps = m_cachedBitmaps->get_item();

	unsigned long long mem = 0;
	for (size_t i = 0; i < cached_bitmaps->size(); i++) {
		auto d = cached_bitmaps->at(i).bitmap.m_object->GetPixelSize();
		mem += static_cast<unsigned long long>(d.height) * d.width * 4;
	}
	return mem;
}

PDFRenderHandler::~PDFRenderHandler() {
	m_display_list_thread.join();
	m_preview_bitmap_thread.join();
}
