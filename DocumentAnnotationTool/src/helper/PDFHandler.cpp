#include "include.h"


Direct2DRenderer::BitmapObject PDFRenderHandler::get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi) const {
	fz_matrix ctm;
	auto size = m_pdf->get_page_size(page, dpi);
	ctm = fz_scale((dpi / MUPDF_DEFAULT_DPI), (dpi / MUPDF_DEFAULT_DPI));

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	Direct2DRenderer::BitmapObject obj;

	auto ctx = m_pdf->get_context();
	auto doc = m_pdf->get_document();
	auto pag = m_pdf->get_page(*doc, page);

	fz_try(*ctx) {
		pixmap = fz_new_pixmap(*ctx, fz_device_rgb(*ctx), size.width, size.height, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, pag, drawdevice, ctm, nullptr);
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
	auto doc = m_pdf->get_document();
	auto pag = m_pdf->get_page(*doc, page);

	fz_try(*ctx) {
		pixmap = fz_new_pixmap_with_bbox(*ctx, fz_device_rgb(*ctx), bbox, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, pag, drawdevice, ctm, nullptr);
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
	auto doc = m_pdf->get_document();

	auto pag = m_pdf->get_page(*doc, page); 

	fz_try(*ctx) {
		pixmap = fz_new_pixmap_with_bbox(*ctx, fz_device_rgb(*ctx), pixmap_size, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background
		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, pag, drawdevice, fz_concat(transform, ctm), nullptr);
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
		auto doc = m_pdf->get_document();
		auto p = m_pdf->get_page(*doc, i);
		fz_try(*ctx) {
			// create a display list with all the draw calls and so on
			list = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));
			dev = fz_new_list_device(*ctx, list);
			// run the device
			fz_run_page(*ctx, p, dev, fz_identity, nullptr);
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
		auto doc = m_pdf->get_document();
		auto p = fz_load_page(copy_ctx, *doc, i);
		fz_try(copy_ctx) {
			// create a display list with all the draw calls and so on
			list = fz_new_display_list(copy_ctx, fz_bound_page(copy_ctx, p));
			dev = fz_new_list_device(copy_ctx, list);
			// run the device
			fz_run_page(copy_ctx, p, dev, fz_identity, nullptr);
			// add list to array
			display_list->push_back(std::shared_ptr<DisplayListWrapper>(new DisplayListWrapper(m_pdf->get_context_wrapper(), list)));
		} fz_always(copy_ctx) {
			// flush the device
			fz_close_device(copy_ctx, dev);
			fz_drop_device(copy_ctx, dev);
		} fz_catch(copy_ctx) {
			ASSERT(false, "Could not create display list"); 
		}
		fz_drop_page(copy_ctx, p);
	}

	fz_drop_context(copy_ctx);
	m_display_list_processed = true;
	m_render_queue_condition_var.notify_all();
	PostMessage(m_window->get_hwnd(), WM_PAINT, (WPARAM)nullptr, (LPARAM) nullptr);
}


PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window, size_t amount_threads) : m_pdf(pdf), m_renderer(renderer), m_window(window) {
	// create member object
	m_display_list = std::shared_ptr<ThreadSafeVector<std::shared_ptr<DisplayListWrapper>>>
		(new ThreadSafeVector<std::shared_ptr<DisplayListWrapper>>(std::vector<std::shared_ptr<DisplayListWrapper>>()));

	m_preview_bitmaps = std::shared_ptr<ThreadSafeVector<CachedBitmap>>
		(new ThreadSafeVector<CachedBitmap>(std::vector<CachedBitmap>()));
	
	m_cachedBitmaps = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>())); 

	m_render_queue = std::shared_ptr<ThreadSafeDeque<std::shared_ptr<RenderInfo>>>
		(new ThreadSafeDeque<std::shared_ptr<RenderInfo>>(std::deque<std::shared_ptr<RenderInfo>>())); 

	m_pagerec = std::shared_ptr<ThreadSafeVector<Renderer::Rectangle<float>>>
		(new ThreadSafeVector<Renderer::Rectangle<float>>(std::vector<Renderer::Rectangle<float>>()));

	// init of m_pagerec
	sort_page_positions();
	// init of m_previewBitmaps
	//create_preview(1); // create preview with default scale


	m_display_list_thread = std::thread([this] { create_display_list_async(); });
	m_preview_bitmap_thread = std::thread([this] { create_preview(0.1); });

	for (size_t i = 0; i < amount_threads; i++) {
		m_render_threads.push_back(std::thread([this] { async_render(); }));
	}
}


void PDFRenderHandler::create_preview(float scale) {
	{
		auto prev = m_preview_bitmaps->get_item();
		prev->clear();
	}

	auto dpi = m_renderer->get_dpi();

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		{
			auto prev = m_preview_bitmaps->get_item();
			auto dest = m_pagerec->get_item(); 
			CachedBitmap p; 
			p.bitmap = get_bitmap(*m_renderer, i, dpi * scale);
			p.dest = dest->at(i);
			p.page = i; 
			p.dpi = dpi;
			prev->push_back(std::move(p));
		}
		m_window->invalidate_drawing_area(); 
	}

	m_preview_scale = scale;
	m_preview_bitmaps_processed = true;
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

void PDFRenderHandler::update_render_queue() {
	// remove items that are finished
	auto queue = m_render_queue->get_item();
	for (int i = queue->size() - 1; i >= 0; i--) {
		if (queue->at(i)->stat == RenderInfo::STATUS::FINISHED or queue->at(i)->stat == RenderInfo::STATUS::ABORTED)
			queue->erase(queue->begin() + i); 
	}
}

void PDFRenderHandler::render_high_res() {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());

	std::vector<std::tuple<size_t, Renderer::Rectangle<float>, std::vector<CachedBitmap*>>> clipped_documents;

	reduce_cache_size(500);

	// TODO
	update_render_queue();

	auto lambda_add_stitch = [](Renderer::Rectangle<float>& r) {
			r.x -= PDF_STITCH_THRESHOLD;
			r.y -= PDF_STITCH_THRESHOLD;
			r.width += PDF_STITCH_THRESHOLD * 2;
			r.height += PDF_STITCH_THRESHOLD * 2;
		};
	
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		// first check if they intersect
		auto dest = m_pagerec->get_item();
		if (dest->at(i).intersects(clip_space)) {
			// this is the overlap in the clip space
			auto overlap = clip_space.calculate_overlap(dest->at(i)); 

			//Check if page overlaps with any chached bitmap
			std::vector<size_t> cached_bitmap_index = render_job_clipped_cashed_bitmaps(overlap, dpi);

			// check if the bitmaps are already queued
			std::vector<Renderer::Rectangle<float>> queued_and_cached_bitmaps;
			auto cached_bitmaps = m_cachedBitmaps->get_item();
			auto render_queue = m_render_queue->get_item();

			queued_and_cached_bitmaps.reserve(render_queue->size());
			for (size_t j = 0; j < render_queue->size(); j++) {
				auto docspace = render_queue->at(j)->overlap_in_docspace;
				docspace.x += dest->at(i).x;
				docspace.y += dest->at(i).y;
				queued_and_cached_bitmaps.push_back(docspace);
			}

			for (size_t j = 0; j < cached_bitmap_index.size(); j++) { 
				queued_and_cached_bitmaps.push_back(cached_bitmaps->at(cached_bitmap_index.at(j)).dest);
			}

			// if there are no cached bitmaps we have to render everything
			if (queued_and_cached_bitmaps.empty()) {
				// add stitch
				lambda_add_stitch(overlap);

				// need to transfom into doc space
				overlap.x -= dest->at(i).x;
				overlap.y -= dest->at(i).y;

				// add to the queue
				render_queue->push_back(std::shared_ptr<RenderInfo>(new RenderInfo({ i, dpi, overlap })));

				continue;
			}

			std::vector<Renderer::Rectangle<float>> chopped_render_targets = splice_rect(overlap, queued_and_cached_bitmaps); 
			merge_rects(chopped_render_targets); 
			remove_small_rects(chopped_render_targets); 

			for (size_t i = 0; i < chopped_render_targets.size(); i++) { 
				chopped_render_targets.at(i).validate(); 
			}

			for (size_t j = 0; j < chopped_render_targets.size(); j++) {
				// transform into doc space
				chopped_render_targets.at(j).x -= dest->at(i).x;
				chopped_render_targets.at(j).y -= dest->at(i).y;

				// make choppy bigger so the are no stitch lines
				lambda_add_stitch(chopped_render_targets.at(j));

				// add the chopped up recs to the queue
				render_queue->push_back(std::shared_ptr<RenderInfo>(new RenderInfo({ i, dpi, chopped_render_targets.at(j) })));
			}

			// add all bitmaps into an array so it can be send to the main window
			std::vector<CachedBitmap*> temp_cachedBitmaps;
			for (size_t i = 0; i < cached_bitmap_index.size(); i++) {
				temp_cachedBitmaps.push_back(&cached_bitmaps->at(cached_bitmap_index.at(i)));
			}
			// we can send some of the cached bitmaps already to the window
			SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
		}
	}


	// ___---___ Multithreaded solution ___---___
	remove_unused_queue_items();
	if (m_amount_thread_running > 0) {
		m_render_queue_condition_var.notify_all();
		return;
	} 

	// ___---___ NON multithreaded solution ___---___ 
	auto dest		    = m_pagerec->get_item(); 
	auto cached_bitmaps = m_cachedBitmaps->get_item();
	auto render_queue   = m_render_queue->get_item();
	std::vector<CachedBitmap*> temp_cachedBitmaps; 
	for (size_t i = 0; i < render_queue->size(); i++) {
		auto page = render_queue->at(i)->page;
		auto overlap = render_queue->at(i)->overlap_in_docspace;
		auto butmap = get_bitmap(*m_renderer, page, overlap, dpi); 
	
		// transform from doc space to clip space
		overlap.x += dest->at(page).x;
		overlap.y += dest->at(page).y;
	
		// add all bitmaps to the cache
		cached_bitmaps->push_back({ std::move(butmap), overlap, dpi, 0 });
		cached_bitmaps->back().page = page;
		// add to an temporary array so it can be send to the main window
		temp_cachedBitmaps.push_back(&cached_bitmaps->back());
	
	}
	
	// send the new rendered bitmaps to the window
	SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
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
	auto page_rec     = m_pagerec->get_item();
	auto render_queue = m_render_queue->get_item();
	auto dpi		  = m_renderer->get_dpi() * m_renderer->get_transform_scale();

	for (int i = render_queue->size() - 1; i >= 0; i--) {
		auto& queue_item = render_queue->at(i);
		auto rec_docspace = queue_item->overlap_in_docspace;
		rec_docspace.x += page_rec->at(queue_item->page).x;
		rec_docspace.y += page_rec->at(queue_item->page).y;
		if (rec_docspace.intersects(clip_space) and FLOAT_EQUAL(queue_item->dpi, dpi)) {
			continue;
		}


		if (queue_item->stat == RenderInfo::STATUS::IN_PROGRESS)
			queue_item->cookie->abort = true;
		else
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
	std::vector<fz_display_list*> cloned_lists; 
	{
		auto list_array = m_display_list->get_item();
		for (size_t i = 0; i < list_array->size(); i++) {
			// retrieve list
			auto list = list_array->at(i)->get_item();

			// do the copy operation
			fz_display_list* ls = fz_new_display_list(ctx, (*list)->mediabox);
			ls->len = (*list)->len;
			ls->max = (*list)->max;
			ls->list = new fz_display_node[ls->max];
			memcpy(ls->list, (*list)->list, (*list)->len * sizeof(fz_display_node));

			cloned_lists.push_back(ls);
		}
	}


	while (!(m_should_threads_die)) {
		// lock to get access to the deque and wait for the conditional variable
		std::unique_lock<std::mutex> lock(m_render_queue_mutex);
		m_render_queue_condition_var.wait(lock, [this] { 
			//get render queue
			auto q = m_render_queue->get_item();
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
			auto queue = m_render_queue->get_item();
			// continue waiting if the queue is empty
			if (queue->size() == 0)
				continue;

			// get the first item that need rendering
			for (size_t i = 0; i < queue->size(); i++) {
				if (queue->at(i)->stat == RenderInfo::STATUS::WAITING) {
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
			// create new pixmap
			pixmap = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), pixmap_size, nullptr, 1);
			fz_clear_pixmap_with_value(ctx, pixmap, 0xff); // for the white background
			// create draw device
			drawdevice = fz_new_draw_device(ctx, fz_concat(transform, ctm), pixmap);
			// render to draw device
			// TODO maybe add a cookie so the rendering can be aborted at any time
			fz_run_display_list(ctx, cloned_lists.at(info->page), drawdevice, fz_identity, info->overlap_in_docspace, info->cookie);
			
			// check if the rendering was aborted
			if (info->cookie->abort == 1) {
				info->stat = RenderInfo::STATUS::ABORTED; 
				break;
			}
			
			fz_close_device(ctx, drawdevice);


			// ___---___ Bitmap creatin and display part ___---___
			// create the bitmap
			auto dest = m_pagerec->get_item();

			auto obj = m_renderer->create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, info->dpi); // default dpi of the pixmap
			
			CachedBitmap bitmap;
			bitmap.bitmap = std::move(obj);
			bitmap.dpi = info->dpi;

			info->overlap_in_docspace.x += dest->at(info->page).x;
			info->overlap_in_docspace.y += dest->at(info->page).y;
			bitmap.dest = info->overlap_in_docspace;

			auto bitmaps_list = m_cachedBitmaps->get_item();
			bitmaps_list->push_back(std::move(bitmap));

			// update info only after no members are called from info 
			info->stat = RenderInfo::STATUS::FINISHED;

			// We have to do a non blocking call since the main thread could be busy waiting for the cached bitmaps
			PostMessage(m_window->get_hwnd(), WM_PAINT, (WPARAM)nullptr, (LPARAM)nullptr);

		} fz_always(ctx) {
			// drop all devices
			fz_drop_device(ctx, drawdevice);
			fz_drop_pixmap(ctx, pixmap);
		} fz_catch(ctx) {
			__debugbreak();
			//return;
		}

	}

	// delete the cloned displaylist
	for (size_t i = 0; i < cloned_lists.size(); i++) { 
		// also not safe and probably not intended
		delete[] cloned_lists.at(i)->list; 
		cloned_lists.at(i)->list = nullptr; 
		cloned_lists.at(i)->len = 0; 
		cloned_lists.at(i)->max = 0; 
		fz_drop_display_list(ctx, cloned_lists.at(i)); 
	}

	fz_drop_context(ctx);
	--m_amount_thread_running;
	m_render_queue_condition_var.notify_all(); 
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
	auto dest = m_pagerec->get_item();
	auto prev = m_preview_bitmaps->get_item();
	// check for intersection. if it intersects than do the rendering
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		if (dest->at(i).intersects(clip_space)) {
			m_renderer->draw_bitmap(prev->at(i).bitmap, dest->at(i), Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
		}
	}
	m_renderer->end_draw();
}

void PDFRenderHandler::render_outline() {
	m_renderer->begin_draw();
	m_renderer->set_current_transform_active();
	// get the clip space.
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size()); 

	auto dest = m_pagerec->get_item();
	for (size_t i = 0; i < dest->size(); i++) { 
		if (dest->at(i).intersects(clip_space)) {
			m_renderer->draw_rect_filled(dest->at(i), {255, 255, 255});
			m_renderer->draw_text(L"Loading page...", dest->at(i).upperleft(), {255, 0, 0}, 20.0f);
		}
	}

	// draww all cached bitmaps
	std::vector<CachedBitmap*> temp;
	{
		// access to cached bitmaps
		auto prev = m_preview_bitmaps->get_item();
		for (size_t i = 0; i < prev->size(); i++) {
			if (prev->at(i).dest.intersects(clip_space)) {
				temp.push_back(&prev->at(i));
			}
		}

		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp));
	}

	m_renderer->end_draw();
}

void PDFRenderHandler::render() {
	auto scale = m_renderer->get_transform_scale();
	//render_preview();

	draw();
	if (scale > m_preview_scale + EPSILON and m_preview_bitmaps_processed) {
		// if we are zoomed in, we need to render the page at a higher resolution
		// than the screen resolution
		render_high_res();
	}
}

void PDFRenderHandler::draw() {
	if (m_preview_bitmaps_processed == false) {
		render_outline();
		return;
	}
	render_preview();

	
	auto cached = m_cachedBitmaps->get_item();
	// add all bitmaps into an array so it can be send to the main window
	std::vector<CachedBitmap*> temp_cachedBitmaps;
	for (size_t i = 0; i < cached->size(); i++) {
		temp_cachedBitmaps.push_back(&cached->at(i));
	}
	// we can send some of the cached bitmaps already to the window
	SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps)); 
}

void PDFRenderHandler::debug_render() {
	auto scale = m_renderer->get_transform_scale();
}

void PDFRenderHandler::sort_page_positions() {
	auto dest = m_pagerec->get_item();
	dest->clear();
	double height = 0;
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		auto size = m_pdf->get_page_size(i);
		dest->push_back(Renderer::Rectangle<double>(0, height, size.width, size.height));
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
	m_should_threads_die = true;
	m_render_queue_condition_var.notify_all();

	for (size_t i = 0; i < m_render_threads.size(); i++) { 
		m_render_threads.at(i).join();
	}

	m_display_list_thread.join();
	m_preview_bitmap_thread.join();
}
