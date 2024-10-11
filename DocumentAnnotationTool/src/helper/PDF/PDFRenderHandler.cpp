#include "PDFRenderHandler.h"

Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi) const {
	fz_matrix ctm;
	auto size = m_pdf->get_page_size(page, dpi);
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	auto ctx = m_pdf->get_context();
	auto doc = m_pdf->get_document();
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

Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, Math::Rectangle<unsigned int> rec) const {
	fz_matrix ctm;
	auto size = m_pdf->get_page_size(page); 
	ctm = fz_scale((rec.width / size.width), (rec.height / size.height));

	// this is to calculate the size of the pixmap
	auto bbox = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, size.width, size.height), ctm));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	auto ctx = m_pdf->get_context();
	auto doc = m_pdf->get_document();
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

Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, Math::Rectangle<float> source, float dpi) const {
	fz_matrix ctm;
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));
	auto transform = fz_translate(-source.x, -source.y);
	// this is to calculate the size of the pixmap using the source
	auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, source.width, source.height), ctm));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	auto ctx = m_pdf->get_context();
	auto doc = m_pdf->get_document();

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
	Logger::log(L"Start creating Display List");
	Timer time;
	// for each rec create a display list
	fz_display_list* list_annot   = nullptr;
	fz_display_list* list_widget  = nullptr;
	fz_display_list* list_content = nullptr;
	fz_device* dev_annot          = nullptr; 
	fz_device* dev_widget         = nullptr; 
	fz_device* dev_content        = nullptr; 

	size_t amount_of_pages = m_pdf->get_page_count();
	m_display_list_amount_processed_total = amount_of_pages;

	// copy the context
	fz_context* copy_ctx = nullptr; 
	{
		auto ctx = m_pdf->get_context(); 
		copy_ctx = fz_clone_context(*ctx); 
	}
	 
	auto display_list = m_display_list->get_write();

	for (size_t i = 0; i < amount_of_pages; i++) { 
		// get the page that will be rendered
		auto doc = m_pdf->get_document();
		// we have to do a fz call since we want to use the ctx.
		// we can't use any other calls (like m_pdf->get_page()) since we would need to create a ctx
		// wrapper which would be overkill for this scenario.
		auto p = fz_load_page(copy_ctx, *doc, static_cast<int>(i));
		fz_try(copy_ctx) {
			// create a display list with all the draw calls and so on
			list_annot   = fz_new_display_list(copy_ctx, fz_bound_page(copy_ctx, p));
			list_widget  = fz_new_display_list(copy_ctx, fz_bound_page(copy_ctx, p));
			list_content = fz_new_display_list(copy_ctx, fz_bound_page(copy_ctx, p));

			dev_annot    = fz_new_list_device(copy_ctx, list_annot);
			dev_widget   = fz_new_list_device(copy_ctx, list_widget);
			dev_content  = fz_new_list_device(copy_ctx, list_content);

			// run all three devices
			Timer time2;
			fz_run_page_annots(copy_ctx, p, dev_annot, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Annots Rendered in ", time2);

			time2 = Timer();
			fz_run_page_widgets(copy_ctx, p, dev_widget, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Widgets Rendered in ", time2);

			time2 = Timer();
			fz_run_page_contents(copy_ctx, p, dev_content, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Content Rendered in ", time2);

			// add list to array
			DisplayListContent c;
			c.m_page_annots  = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_annot)); 
			c.m_page_widgets = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_widget));
			c.m_page_content = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_content));
			display_list->push_back(c);
		} fz_always(copy_ctx) {
			// flush the device
			fz_close_device(copy_ctx, dev_annot);
			fz_close_device(copy_ctx, dev_widget);
			fz_close_device(copy_ctx, dev_content); 
			fz_drop_device(copy_ctx, dev_annot);
			fz_drop_device(copy_ctx, dev_widget); 
			fz_drop_device(copy_ctx, dev_content); 
		} fz_catch(copy_ctx) {
			ASSERT(false, "Could not create display list");
		}
		// always drop page at the end
		fz_drop_page(copy_ctx, p);

		m_display_list_amount_processed = m_display_list_amount_processed + 1;
		PostMessage(m_window->get_hwnd(), WM_CUSTOM_MESSAGE, (LPARAM)nullptr, CUSTOM_WM_MESSAGE::PDF_HANDLER_DISPLAY_LIST_UPDATE);
	}


	fz_drop_context(copy_ctx); 
	m_display_list_processed = true;
	m_render_queue_condition_var.notify_all(); 
	PostMessage(m_window->get_hwnd(), WM_PAINT, (WPARAM)nullptr, (LPARAM) nullptr); 
	PostMessage(m_window->get_hwnd(), WM_CUSTOM_MESSAGE, (LPARAM)nullptr, CUSTOM_WM_MESSAGE::PDF_HANDLER_DISPLAY_LIST_UPDATE);
	Logger::log(L"Finished Displaylist in ", time);
}

void PDFRenderHandler::create_display_list(size_t page) {
	// for each rec create a display list
	fz_display_list* list_annot = nullptr;
	fz_display_list* list_widget = nullptr;
	fz_display_list* list_content = nullptr;
	fz_device* dev_annot = nullptr;
	fz_device* dev_widget = nullptr;
	fz_device* dev_content = nullptr;

	auto display_list = m_display_list->get_write();

	// get the page that will be rendered
	auto doc = m_pdf->get_document();
	auto ctx = m_pdf->get_context();
	// we have to do a fz call since we want to use the ctx.
	// we can't use any other calls (like m_pdf->get_page()) since we would need to create a ctx
	// wrapper which would be overkill for this scenario.
	auto p = fz_load_page(*ctx, *doc, static_cast<int>(page)); 
	fz_try(*ctx) {
		// create a display list with all the draw calls and so on
		list_annot = fz_new_display_list(*ctx, fz_bound_page(*ctx, p)); 
		list_widget = fz_new_display_list(*ctx, fz_bound_page(*ctx, p)); 
		list_content = fz_new_display_list(*ctx, fz_bound_page(*ctx, p)); 

		dev_annot = fz_new_list_device(*ctx, list_annot);
		dev_widget = fz_new_list_device(*ctx, list_widget);
		dev_content = fz_new_list_device(*ctx, list_content);

		// run all three devices
		fz_run_page_annots(*ctx, p, dev_annot, fz_identity, nullptr);
		fz_run_page_widgets(*ctx, p, dev_widget, fz_identity, nullptr);
		fz_run_page_contents(*ctx, p, dev_content, fz_identity, nullptr);

		// add list to array
		DisplayListContent c;
		c.m_page_annots = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_annot));
		c.m_page_widgets = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_widget));
		c.m_page_content = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_content));

		if (display_list->begin() + page == display_list->end()) {
			display_list->push_back(c); 
		}
		else {
			display_list->insert(display_list->begin() + page, c); 
		}
	} fz_always(*ctx) {
		// flush the device
		fz_close_device(*ctx, dev_annot);
		fz_close_device(*ctx, dev_widget);
		fz_close_device(*ctx, dev_content);
		fz_drop_device(*ctx, dev_annot);
		fz_drop_device(*ctx, dev_widget);
		fz_drop_device(*ctx, dev_content);
	} fz_catch(*ctx) {
		ASSERT(false, "Could not create display list");
	}
	// always drop page at the end
	fz_drop_page(*ctx, p);

}

PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window, size_t amount_threads) : m_pdf(pdf), m_renderer(renderer), m_window(window) {
	// --- Render queue ---
	m_render_queue = std::shared_ptr<ThreadSafeDeque<std::shared_ptr<RenderInfo>>>
		(new ThreadSafeDeque<std::shared_ptr<RenderInfo>>(std::deque<std::shared_ptr<RenderInfo>>())); 
	// --- Display list generation ---
	m_display_list = std::shared_ptr<ThreadSafeVector<DisplayListContent>>
		(new ThreadSafeVector<DisplayListContent>(std::vector<DisplayListContent>())); 
	// --- High res rendering ---
	m_highres_bitmaps = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>())); 
	// --- Preview generation ---
	m_preview_bitmaps = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>()));
	// --- Annotation generation ---
	m_annotation_bitmaps = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>()));
	// empy list
	m_annotation_bitmaps_old = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>()));

	

	// We need to immediately start the display list thread
	m_display_list_thread = std::thread([this] { create_display_list(); });

	for (size_t i = 0; i < amount_threads; i++) {
		m_render_threads.push_back(std::thread([this] { async_render(); }));
	}
}

template<typename T>
void remove_small_rects(std::vector <Math::Rectangle<T>>& rects, float threshold = EPSILON) {
	for (long i = static_cast<long>(rects.size() - 1); i >= 0; i--) {
		rects.at(i).validate();
		if (rects.at(i).width < threshold or rects.at(i).height < threshold) {
			rects.erase(rects.begin() + i);
		}
	}
}

void PDFRenderHandler::update_render_queue() {
	// remove items that are finished
	auto queue = m_render_queue->get_write();
	for (long i = static_cast<long>(queue->size() - 1); i >= 0; i--) {
		if (queue->at(i)->stat == RenderInfo::STATUS::FINISHED or queue->at(i)->stat == RenderInfo::STATUS::ABORTED or 
			(queue->at(i)->job == RenderInfo::JOB_TYPE::RELOAD_ANNOTATIONS and queue->at(i)->thread_ids.size() == m_amount_thread_running)) 
			queue->erase(queue->begin() + i); 
	}
}

void PDFRenderHandler::create_render_job(RenderInstructions r) {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	update_render_queue();

	// --- High res rendering ---
	if (dpi > m_preview_dpi + EPSILON and r.render_highres) {
		reduce_cache_size(m_highres_bitmaps, 100);
		std::vector<size_t> cached_bitmap_index; 
		// get the overlaps
		auto overlaps = get_pdf_overlap(RenderInfo::JOB_TYPE::HIGH_RES, cached_bitmap_index, dpi, 1.5f, 10); 

		auto queue = m_render_queue->get_write();
		for (size_t i = 0; i < overlaps.size(); i++) {
			auto render_info = std::make_shared<RenderInfo>(std::move(overlaps.at(i)));  
			queue->push_back(render_info); 
		}
	}

	// --- Preivew rendering ---
    if (r.render_preview) {
        auto pages = get_page_overlap();
        auto preview = m_preview_bitmaps->get_read();
        auto queue = m_render_queue->get_write();
        auto rec = m_pdf->get_pagerec()->get_read();

        for (const auto& page : pages) {
            bool isCached = false;
            bool isInQueue = false;

            // Check if the page is already cached
            for (const auto& bitmap : *preview) {
                if (bitmap.page == page) {
                    isCached = true;
                    break;
                }
            }

            // Check if the page is already in the render queue
            for (const auto& q : *queue) {
                if (q->page == page and q->job == RenderInfo::JOB_TYPE::PREVIEW) {
                    isInQueue = true;
                    break;
                }
            }

            // If the page is not cached and not in the queue, create a render job
            if (!isCached and !isInQueue) {
				auto renderInfo = std::make_shared<RenderInfo>(page, m_preview_dpi, rec->at(page) - rec->at(page).upperleft(), RenderInfo::JOB_TYPE::PREVIEW);
				queue->push_back(std::move(renderInfo));
            }
        }
    }

	// --- Annots rendering ---
	if (dpi > m_preview_dpi + EPSILON and r.render_annots) {
		reduce_cache_size(m_annotation_bitmaps, 100);
		std::vector<size_t> cached_bitmap_index; 
		// get the overlaps
		auto overlaps = get_pdf_overlap(RenderInfo::JOB_TYPE::ANNOTATION, cached_bitmap_index, dpi, 0, 10);

		auto queue = m_render_queue->get_write();
		for (size_t i = 0; i < overlaps.size(); i++) { 
			auto render_info = std::make_shared<RenderInfo>(std::move(overlaps.at(i))); 
			queue->push_back(render_info); 
		}
	}

	remove_unused_queue_items();
	if (m_amount_thread_running > 0) {
		m_render_queue_condition_var.notify_all();
		return;
	}
}

std::vector<PDFRenderHandler::RenderInfo> PDFRenderHandler::get_pdf_overlap(RenderInfo::JOB_TYPE type, std::vector<size_t>& cached_bitmap_index, std::optional<float> dpi_check, float dpi_margin, size_t limit) {
	std::vector<PDFRenderHandler::RenderInfo> end_overlap;
	// We first get the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());

	auto add_stitch = [](Math::Rectangle<float>& r) {
		r.x      -= PDF_STITCH_THRESHOLD;
		r.y      -= PDF_STITCH_THRESHOLD;
		r.width  += PDF_STITCH_THRESHOLD * 2;
		r.height += PDF_STITCH_THRESHOLD * 2;
	};

	size_t page_count = m_pdf->get_page_count();
	auto page_recs    = m_pdf->get_pagerec()->get_read();

	std::shared_ptr<ThreadSafeDeque<CachedBitmap>> cached_bitmaps_ptr = nullptr;
	switch (type) {
	case PDFRenderHandler::RenderInfo::HIGH_RES:
		cached_bitmaps_ptr = m_highres_bitmaps;
		break;
	case PDFRenderHandler::RenderInfo::PREVIEW:
		cached_bitmaps_ptr = m_preview_bitmaps;
		break;
	case PDFRenderHandler::RenderInfo::ANNOTATION:
		cached_bitmaps_ptr = m_annotation_bitmaps;
		break;
	}
	ASSERT(cached_bitmaps_ptr != nullptr, "Cached bitmaps is nullptr"); 

	for (size_t curr_page = 0; curr_page < page_count; curr_page++) {
		// If it doesn't intersect we can skip it
		if (page_recs->at(curr_page).intersects(clip_space) == false) {
			continue;
		}

		auto page_clip_overlap = clip_space.calculate_overlap(page_recs->at(curr_page));
		std::vector<Math::Rectangle<float>> intersecting_recs;

		// --- Check if the page is already in the cache ---
		auto cached_bitmaps = cached_bitmaps_ptr->get_read();
		// The index of all cached bitmaps that overlap
		for (size_t j = 0; j < cached_bitmaps->size(); j++) {
			auto clipspace = cached_bitmaps->at(j).doc_coords + page_recs->at(cached_bitmaps->at(j).page).upperleft();
			// check if it overlaps
			if (clipspace.intersects(page_clip_overlap) == false) 
				continue;
			 
			// check if dpi check is needed
			if (dpi_check.has_value() == false)
				continue;

			// Exact check of DPI
			if (FLOAT_EQUAL(0, dpi_margin)) {
				if (FLOAT_EQUAL(cached_bitmaps->at(j).dpi, dpi_check.value()) == false) {
					continue;
				}
			}
			// DPI check with a margin
			else {
				// check if the dpi is high enough
				if (cached_bitmaps->at(j).dpi < dpi_check.value() and FLOAT_EQUAL(cached_bitmaps->at(j).dpi, dpi_check.value()) == false)
					continue;
				// also check if the dpi is too high
				if (cached_bitmaps->at(j).dpi > dpi_check.value() * dpi_margin /* Magic Number */)
					continue;
			}


			cached_bitmap_index.push_back(j);
			intersecting_recs.push_back(clipspace);
		}

		// --- Check if the page is already in the render queue ---
		auto render_queue = m_render_queue->get_read();
		for (size_t j = 0; j < render_queue->size(); j++) {
			// First check if the job type is compatible or if the page is aborted
			if (render_queue->at(j)->job != type or render_queue->at(j)->cookie->abort == 1) 
				continue;

			// put into clip space
			auto clipspace = render_queue->at(j)->overlap_in_docspace + page_recs->at(render_queue->at(j)->page).upperleft();
			// check if it overlaps
			if (clipspace.intersects(page_clip_overlap) == false)
				continue;

			// check if dpi check is needed
			if (dpi_check.has_value() == false)
				continue;

			// Exact check of DPI
			if (FLOAT_EQUAL(0, dpi_margin)) {
				if (FLOAT_EQUAL(render_queue->at(j)->dpi, dpi_check.value()) == false) {
					continue;
				}
			}
			// DPI check with a margin
			else {
				// check if the dpi is high enough
				if (render_queue->at(j)->dpi < dpi_check.value() and FLOAT_EQUAL(render_queue->at(j)->dpi, dpi_check.value()) == false)
					continue;
				// also check if the dpi is too high
				if (render_queue->at(j)->dpi > dpi_check.value() * dpi_margin /* Magic Number */)
					continue;
			}

			intersecting_recs.push_back(clipspace);
		}

		// --- Add to render queue ---
		// if there were no intersecting recs we need to render the whole page
		if (intersecting_recs.empty()) {
			// first transform into docspace
			auto temp_docspace = page_clip_overlap - page_recs->at(curr_page).upperleft();
			// and then add the stitch
			add_stitch(temp_docspace); 
			// add to vector
			RenderInfo info(curr_page, dpi_check.value_or(MUPDF_DEFAULT_DPI), temp_docspace, type);
			end_overlap.push_back(std::move(info));
			continue;
		}
		
		// in the other case we have to process the overlap further for more efficient rendering
		std::vector<Math::Rectangle<float>> chopped_render_targets = splice_rect(page_clip_overlap, intersecting_recs);
		merge_rects(chopped_render_targets); 
		remove_small_rects(chopped_render_targets); 

		if (chopped_render_targets.size() >= limit) {
			// first transform into docspace
			auto temp_docspace = page_clip_overlap - page_recs->at(curr_page).upperleft(); 
			// and then add the stitch
			add_stitch(temp_docspace); 
			// add to vector
			RenderInfo info(curr_page, dpi_check.value_or(MUPDF_DEFAULT_DPI), temp_docspace, type); 
			end_overlap.push_back(std::move(info)); 
			continue;
		}

		// And validate
		for (size_t j = 0; j < chopped_render_targets.size(); j++) {
			chopped_render_targets.at(j).validate();

			// first transform into docspace
			auto temp_docspace = chopped_render_targets.at(j) - page_recs->at(curr_page).upperleft(); 
			// and then add the stitch
			add_stitch(temp_docspace);
			// add to vector
			RenderInfo info(curr_page, dpi_check.value_or(MUPDF_DEFAULT_DPI), temp_docspace, type);  
			end_overlap.push_back(std::move(info));
		}
	}

	return end_overlap;
}

std::vector<size_t> PDFRenderHandler::get_page_overlap() {
	std::vector<size_t> pages;
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());
	size_t page_count = m_pdf->get_page_count(); 
	auto page_recs = m_pdf->get_pagerec()->get_read(); 

	for (size_t curr_page = 0; curr_page < page_count; curr_page++) {
		// If it doesn't intersect we can skip it
		if (page_recs->at(curr_page).intersects(clip_space) == false) {
			continue;
		}
		pages.push_back(curr_page); 
	}

	return pages; 
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

void PDFRenderHandler::remove_unused_queue_items() {
	auto clip_space   = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());
	auto page_rec     = m_pdf->get_pagerec()->get_read();
	auto render_queue = m_render_queue->get_write();
	auto dpi		  = m_renderer->get_dpi() * m_renderer->get_transform_scale();

	for (long i = static_cast<long>(render_queue->size() - 1); i >= 0; i--) {
		auto& queue_item = render_queue->at(i);
		auto doc_in_clipspace = queue_item->overlap_in_docspace + page_rec->at(queue_item->page).upperleft();
		if ((doc_in_clipspace.intersects(clip_space) and FLOAT_EQUAL(queue_item->dpi, dpi))
			or queue_item->job == RenderInfo::JOB_TYPE::PREVIEW) { 
			continue;
		}

		if (queue_item->stat == RenderInfo::STATUS::IN_PROGRESS)
			queue_item->cookie->abort = true;
		else if (queue_item->job != RenderInfo::JOB_TYPE::RELOAD_ANNOTATIONS) 
			render_queue->erase(render_queue->begin() + i);
	}
}

void PDFRenderHandler::async_render() {
	++m_amount_thread_running;
	// clone context
	fz_context* ctx;
	{
		auto c = m_pdf->get_context();
		ctx = fz_clone_context(*c);
	}
	
	// needs a check to see if the display_list are already processed
	if (m_display_list_processed == false) {
		std::unique_lock<std::mutex> lock(m_render_queue_mutex); 
		m_render_queue_condition_var.wait(lock, [this] { return m_display_list_processed == true or m_should_threads_die; });
	}

	if (m_should_threads_die) {
		fz_drop_context(ctx);
		--m_amount_thread_running;
		m_render_queue_condition_var.notify_all();
		return;
	}

	// clone displaylist (!!Very sketch!!)
	struct ClonedDisplayList {
		fz_display_list* content = nullptr;
		fz_display_list* annots = nullptr;
		fz_display_list* widgets = nullptr;
	};


	auto copy_list = [ctx](fz_display_list* list) { 
		fz_display_list* ls = fz_new_display_list(ctx, (list)->mediabox); 
		ls->len = (list)->len;
		ls->max = (list)->max;
		ls->list = new fz_display_node[ls->max];
		memcpy(ls->list, (list)->list, (list)->len * sizeof(fz_display_node));
		return ls;
	};

	auto delete_list = [ctx](fz_display_list* list) {
		delete[] list->list;
		list->list = nullptr;
		list->len = 0;
		list->max = 0;
		fz_drop_display_list(ctx, list);
		};

	std::vector<ClonedDisplayList> cloned_lists;
	{
		auto list_array = m_display_list->get_read();
		for (size_t i = 0; i < list_array->size(); i++) {
			// add them to an array
			cloned_lists.push_back({ copy_list(*(list_array->at(i).m_page_content.get()->get_item())),  
								     copy_list(*(list_array->at(i).m_page_annots.get() ->get_item())),  
								     copy_list(*(list_array->at(i).m_page_widgets.get()->get_item())) }); 
		}
	}

	while (!(m_should_threads_die)) {
		// lock to get access to the deque and wait for the conditional variable
		std::unique_lock<std::mutex> lock(m_render_queue_mutex);
		m_render_queue_condition_var.wait(lock, [this] { 
			//get render queue
			auto q = m_render_queue->get_read();
			return q->size() != 0 or m_should_threads_die;
		});
		lock.unlock();

		// check if the thread should be dead
		if (m_should_threads_die) {
			break;
		}

		// get the item out of the dequeue
		std::shared_ptr<RenderInfo> info = nullptr;  
		{
			auto queue = m_render_queue->get_write();
			// continue waiting if the queue is empty
			if (queue->size() == 0)
				continue;

			// get the first item that need rendering
			for (size_t i = 0; i < queue->size(); i++) {
				// --- Here we can check what item is in the queue and only look at the ones that need to be processed ---
				if (queue->at(i)->job == RenderInfo::JOB_TYPE::RELOAD_ANNOTATIONS) {
					// check if this thread already reloaded the annotations
					bool already_reloaded = false;
					for (size_t j = 0; j < queue->at(i)->thread_ids.size(); j++) {
						if (queue->at(i)->thread_ids.at(j) == std::this_thread::get_id()) { 
							already_reloaded = true;
							break;
						}
					}
					if (already_reloaded) 
						continue;

					// copy the annotations and widgets again
					auto list_array = m_display_list->get_read();
					delete_list(cloned_lists.at(queue->at(i)->page).annots);
					delete_list(cloned_lists.at(queue->at(i)->page).widgets);

					cloned_lists.at(queue->at(i)->page).annots = copy_list(*(list_array->at(queue->at(i)->page).m_page_annots.get()->get_item()));
					cloned_lists.at(queue->at(i)->page).widgets = copy_list(*(list_array->at(queue->at(i)->page).m_page_widgets.get()->get_item())); 

					// at the end add the id to the render queue
					queue->at(i)->thread_ids.push_back(std::this_thread::get_id());
				} 
				else if (queue->at(i)->job == RenderInfo::JOB_TYPE::RELOAD_DISPLAY_LIST) {
					// check if this thread already reloaded the annotations
					bool already_reloaded = false; 
					for (size_t j = 0; j < queue->at(i)->thread_ids.size(); j++) {
						if (queue->at(i)->thread_ids.at(j) == std::this_thread::get_id()) {
							already_reloaded = true;
							break;
						}
					}
					if (already_reloaded)
						continue;

					auto list_array = m_display_list->get_read();
					// reload the entire array
					for (size_t j = 0; j < cloned_lists.size(); j++) {
						delete_list(cloned_lists.at(j).content);
						delete_list(cloned_lists.at(j).annots);
						delete_list(cloned_lists.at(j).widgets);
					}

					cloned_lists.clear();
					cloned_lists.reserve(list_array->size());

					for (size_t j = 0; j < list_array->size(); j++) {
						cloned_lists.push_back({ copy_list(*(list_array->at(j).m_page_content.get()->get_item())),  
												 copy_list(*(list_array->at(j).m_page_annots. get()->get_item())),  
												 copy_list(*(list_array->at(j).m_page_widgets.get()->get_item())) }); 
					}
					
				}
				else if (queue->at(i)->stat == RenderInfo::STATUS::WAITING) {
					info = queue->at(i);
					info->stat = RenderInfo::STATUS::IN_PROGRESS;
					break;
				}
			}

			// if nothing is found just wait again
			if (info == nullptr)
				continue;
		}

		
		// now we can render it
		fz_matrix ctm;
		ctm = fz_scale((info->dpi / MUPDF_DEFAULT_DPI), (info->dpi / MUPDF_DEFAULT_DPI));
		auto transform = fz_translate(-info->overlap_in_docspace.x, -info->overlap_in_docspace.y);
		// this is to calculate the size of the pixmap using the source
		auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, info->overlap_in_docspace.width, info->overlap_in_docspace.height), ctm));

		fz_pixmap* pixmap = nullptr;
		fz_device* drawdevice = nullptr;

		fz_try(ctx) {
			// ___---___ Rendering part ___---___
			Timer time;
			// create new pixmap
			pixmap = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), pixmap_size, nullptr, 1);
			// create draw device
			drawdevice = fz_new_draw_device(ctx, fz_concat(transform, ctm), pixmap); 
			// render to draw device
			if (info->job == RenderInfo::JOB_TYPE::ANNOTATION) {
				fz_clear_pixmap(ctx, pixmap); // for transparent background
				fz_run_display_list(ctx, cloned_lists.at(info->page).annots, drawdevice, fz_identity, info->overlap_in_docspace, info->cookie);
				fz_run_display_list(ctx, cloned_lists.at(info->page).widgets, drawdevice, fz_identity, info->overlap_in_docspace, info->cookie);
			}
			else {
				fz_clear_pixmap_with_value(ctx, pixmap, 0xff); // for the white background
				fz_run_display_list(ctx, cloned_lists.at(info->page).content, drawdevice, fz_identity, info->overlap_in_docspace, info->cookie);
			}
						
			// check if the rendering was aborted
			if (info->cookie->abort == 1) {
				info->stat = RenderInfo::STATUS::ABORTED; 
				// We dont care if we close the device since everything will be freed anyway
				break;
			}

			fz_close_device(ctx, drawdevice); 
			// ___---___ Bitmap creatin and display part ___---___
			// create the bitmap
			auto dest = m_pdf->get_pagerec()->get_read();

			auto obj = m_renderer->create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, info->dpi); // default dpi of the pixmap
			
			CachedBitmap bitmap;
			bitmap.bitmap = std::move(obj);
			bitmap.dpi = info->dpi;
			bitmap.doc_coords = info->overlap_in_docspace;
			bitmap.page = info->page;

			if (info->job == RenderInfo::JOB_TYPE::HIGH_RES) {
				auto bitmaps_list = m_highres_bitmaps->get_write();
				bitmaps_list->push_back(std::move(bitmap));
			}
			else if (info->job == RenderInfo::JOB_TYPE::PREVIEW) {
				auto bitmaps_list = m_preview_bitmaps->get_write();
				bitmaps_list->push_back(std::move(bitmap)); 
			}
			else if (info->job == RenderInfo::JOB_TYPE::ANNOTATION) {
				auto bitmaps_list = m_annotation_bitmaps->get_write(); 
				bitmaps_list->push_back(std::move(bitmap));  
			}
		

			// update info only after no members are called from info 
			info->stat = RenderInfo::STATUS::FINISHED;

			// time it
			if (time.delta_s() > 2) {
				Logger::warn("Rendering page ", info->page + 1, " took ", time);
			}

			// We have to do a non blocking call since the main thread could be busy waiting for the cached bitmaps
			PostMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)nullptr);

		} fz_always(ctx) {
			// drop all devices
			fz_drop_device(ctx, drawdevice);
			fz_drop_pixmap(ctx, pixmap);
		} fz_catch(ctx) {
			if (info->stat == RenderInfo::STATUS::ABORTED) {
				fz_ignore_error(ctx);
			}
			else {
				ASSERT(false, "MUPdf Error: ", fz_convert_error(ctx, &ctx->error.errcode));
			}
			//return;
		}

	}

	// delete the cloned displaylist
	for (size_t i = 0; i < cloned_lists.size(); i++) { 
		// also not safe and probably not intended

		delete_list(cloned_lists.at(i).content); 
		delete_list(cloned_lists.at(i).annots); 
		delete_list(cloned_lists.at(i).widgets); 
	}

	fz_drop_context(ctx);
	--m_amount_thread_running;
	m_render_queue_condition_var.notify_all(); 
}

float PDFRenderHandler::get_display_list_progress() {
	return (float)((double)m_display_list_amount_processed / (double)m_display_list_amount_processed_total); 
}

void PDFRenderHandler::remove_small_cached_bitmaps(float treshold) {
	auto cached_bitmaps = m_highres_bitmaps->get_write();
	if (cached_bitmaps->size() == 0)
		return;

	for (long i = static_cast<long>(cached_bitmaps->size() - 1); i >= 0; i--) {
		if ((cached_bitmaps->at(i).doc_coords.width < treshold or cached_bitmaps->at(i).doc_coords.height < treshold) and cached_bitmaps->at(i).used == 0) {
			cached_bitmaps->erase(cached_bitmaps->begin() + i);
		}
	}
}

void PDFRenderHandler::reduce_cache_size(std::shared_ptr<ThreadSafeDeque<CachedBitmap>> target, unsigned long long max_memory) {
	auto cached_bitmaps = target->get_write(); 

	//remove_small_cached_bitmaps(50 / m_renderer->get_transform_scale()); 
	auto mem_usage = get_cache_memory_usage(target)/1000000; 
	
	while (mem_usage > max_memory) {
		cached_bitmaps->pop_front();
		mem_usage = get_cache_memory_usage(target) / 1000000;
	}
}

void PDFRenderHandler::render_preview() {
	// draw all cached bitmaps
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	std::vector<CachedBitmap*> temp;
	{
		// access to cached bitmaps
		auto prev = m_preview_bitmaps->get_write();
		auto pagerec = m_pdf->get_pagerec()->get_read();
		for (size_t i = 0; i < prev->size(); i++) {
			if ((prev->at(i).doc_coords + pagerec->at(i).upperleft()).intersects(clip_space)) {
				temp.push_back(&prev->at(i));
			}
		}

		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READ, (WPARAM)nullptr, (LPARAM)(&temp));
	}
}

void PDFRenderHandler::render_outline() {
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size()); 

	{
		// This draws the outline of the pages
		auto dest = m_pdf->get_pagerec()->get_read();
		for (size_t i = 0; i < dest->size(); i++) {
			if (dest->at(i).intersects(clip_space)) {
				m_renderer->draw_rect_filled(dest->at(i), { 255, 255, 255 });
				m_renderer->draw_text(L"Loading page...", dest->at(i).upperleft(), { 255, 0, 0 }, 20.0f);
			}
		}
	}
	m_renderer->end_draw();
}

void PDFRenderHandler::send_bitmaps(RenderInstructions r) { 
	if (r.draw_outline) {
		render_outline();
	}
	if (r.draw_preview) {
		auto cached = m_preview_bitmaps->get_write();
		// add all bitmaps into an array so it can be send to the main window
		std::vector<CachedBitmap*> temp_cachedBitmaps;
		for (size_t i = 0; i < cached->size(); i++) {
			temp_cachedBitmaps.push_back(&cached->at(i));
		}
		// we can send some of the cached bitmaps already to the window
		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READ, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
	}

	if (r.draw_content) {
		auto cached = m_highres_bitmaps->get_write();
		// add all bitmaps into an array so it can be send to the main window
		std::vector<CachedBitmap*> temp_cachedBitmaps;
		for (size_t i = 0; i < cached->size(); i++) {
			temp_cachedBitmaps.push_back(&cached->at(i));
		}
		// we can send some of the cached bitmaps already to the window
		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READ, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
	}

	if (r.draw_annots) {
		auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
		auto cached = m_annotation_bitmaps->get_write();
		bool using_old_cache = false;
	OLD_CACHED_TRY_AGAIN:
		// add all bitmaps into an array so it can be send to the main window
		std::vector<CachedBitmap*> temp_cachedBitmaps;
		for (size_t i = 0; i < cached->size(); i++) {
			if (FLOAT_EQUAL(cached->at(i).dpi, dpi)) {
				temp_cachedBitmaps.push_back(&cached->at(i));
			}
		}
		// add some low res alternatives until newer once are available
		if (temp_cachedBitmaps.empty()) { 
			// for this we search for the next highest dpi in relation to our target dpi
			float highest_dpi = 0.0f;
			for (size_t i = 0; i < cached->size(); i++) {
				if (cached->at(i).dpi > highest_dpi and cached->at(i).dpi < dpi) {
					highest_dpi = cached->at(i).dpi;
				}
			}

			// then add them
			for (size_t i = 0; i < cached->size(); i++) { 
				if (FLOAT_EQUAL(cached->at(i).dpi, highest_dpi)) { 
					temp_cachedBitmaps.push_back(&cached->at(i)); 
				}
			}
		}

		if (temp_cachedBitmaps.empty()) {
			// if its still empty just add anything
			for (size_t i = 0; i < cached->size(); i++) {
				temp_cachedBitmaps.push_back(&cached->at(i));
			}
		}

		if (temp_cachedBitmaps.empty() and using_old_cache == false) {
			// try it with the old cache
			cached = m_annotation_bitmaps_old->get_write();
			using_old_cache = true;
			goto OLD_CACHED_TRY_AGAIN; 
		}

		if (using_old_cache == false) {
			// if we have found bitmaps that we can put into the window
			// we can delete the old cache
			auto temp = m_annotation_bitmaps_old->get_write();
			temp->clear();
		}

		// we can send some of the cached bitmaps already to the window
		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READ, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
	}
}


void PDFRenderHandler::render() {
	render(RenderInstructions());
}

void PDFRenderHandler::render(RenderInstructions instruct) {
	if (m_display_list_processed) {
		create_render_job(instruct);
	}
}

void PDFRenderHandler::clear_render_cache() {
	auto cached = m_highres_bitmaps->get_write();
	cached->clear();
}

void PDFRenderHandler::clear_annotation_cache() {
	m_annotation_bitmaps_old = std::move(m_annotation_bitmaps);

	// create new list
	m_annotation_bitmaps = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>()));
}

void PDFRenderHandler::update_annotations(size_t page) {
	// for each rec create a display list
	fz_display_list* list_annot = nullptr;
	fz_display_list* list_widget = nullptr;
	fz_device* dev_annot = nullptr;
	fz_device* dev_widget = nullptr;

	{
		auto display_list = m_display_list->get_write();

		// get the page that will be rendered
		auto doc = m_pdf->get_document();
		auto p = m_pdf->get_page(page);
		auto ctx = m_pdf->get_context();

		fz_try(*ctx) {
			// create a display list with all the draw calls and so on
			list_annot = fz_new_display_list(*ctx, fz_bound_page(*ctx, *p));
			list_widget = fz_new_display_list(*ctx, fz_bound_page(*ctx, *p));

			dev_annot = fz_new_list_device(*ctx, list_annot);
			dev_widget = fz_new_list_device(*ctx, list_widget);
		
			// run all devices
			fz_run_page_annots(*ctx, *p, dev_annot, fz_identity, nullptr);
			fz_run_page_widgets(*ctx, *p, dev_widget, fz_identity, nullptr);

			// add list to array
			display_list->at(page).m_page_annots = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_annot));
			display_list->at(page).m_page_widgets = std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list_widget));
		} fz_always(*ctx) {
			// flush the device
			fz_close_device(*ctx, dev_annot);
			fz_close_device(*ctx, dev_widget);
			fz_drop_device(*ctx, dev_annot);
			fz_drop_device(*ctx, dev_widget);
		} fz_catch(*ctx) {
			ASSERT(false, "Could not create display list");
		}
	}

	auto queue = m_render_queue->get_write();
	RenderInfo info;
	info.job = RenderInfo::JOB_TYPE::RELOAD_ANNOTATIONS; 
	info.page = page;
	queue->push_back(std::make_shared<RenderInfo>(std::move(info))); 
	m_render_queue_condition_var.notify_all(); 
	clear_annotation_cache();
}

void PDFRenderHandler::update_displaylist(const MuPDFHandler::DisplaylistChangeInfo* info) {
	using CHANGE_TYPE = MuPDFHandler::DisplaylistChangeInfo::CHANGE_TYPE;
	for (const auto& page : info->page_info) {
		switch (page.type) {
		case CHANGE_TYPE::ADDITION:
		{
			create_display_list(page.page1);

			auto queue = m_render_queue->get_write();
			RenderInfo info; 
			info.job = RenderInfo::JOB_TYPE::RELOAD_DISPLAY_LIST;  
			info.page = page.page1; 
			queue->push_back(std::make_shared<RenderInfo>(std::move(info)));
			 
			m_render_queue_condition_var.notify_all(); 
		}
		}
	}
}

unsigned long long PDFRenderHandler::get_cache_memory_usage(std::shared_ptr<ThreadSafeDeque<CachedBitmap>> target) const {
	auto cached_bitmaps = target->get_write();

	unsigned long long mem = 0;
	for (size_t i = 0; i < cached_bitmaps->size(); i++) {
		auto d = cached_bitmaps->at(i).bitmap.m_object->GetPixelSize();
		mem += static_cast<unsigned long long>(d.height) * d.width * 4;
	}
	return mem;
}

PDFRenderHandler::~PDFRenderHandler() {
	m_should_threads_die = true;
	m_render_queue_condition_var.notify_all();

	for (size_t i = 0; i < m_render_threads.size(); i++) { 
		m_render_threads.at(i).join();
	}

	if (m_display_list_thread.joinable())
		m_display_list_thread.join();
}
