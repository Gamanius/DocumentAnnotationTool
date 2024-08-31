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
	Logger::log(L"Start creating Display List");
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
			fz_run_page_annots(copy_ctx, p, dev_annot, fz_identity, nullptr);
			fz_run_page_widgets(copy_ctx, p, dev_widget, fz_identity, nullptr);
			fz_run_page_contents(copy_ctx, p, dev_content, fz_identity, nullptr);

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
		PostMessage(m_window->get_hwnd(), WM_CUSTOM_MESSAGE, CUSTOM_WM_MESSAGE::PDF_HANDLER_DISPLAY_LIST_UPDATE, (LPARAM)nullptr); 
	}


	fz_drop_context(copy_ctx); 
	m_display_list_processed = true;
	m_render_queue_condition_var.notify_all(); 
	PostMessage(m_window->get_hwnd(), WM_PAINT, (WPARAM)nullptr, (LPARAM) nullptr); 
	PostMessage(m_window->get_hwnd(), WM_CUSTOM_MESSAGE, CUSTOM_WM_MESSAGE::PDF_HANDLER_DISPLAY_LIST_UPDATE, (LPARAM)nullptr); 
	Logger::log(L"Finished Displaylist");
}

PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window, size_t amount_threads) : m_pdf(pdf), m_renderer(renderer), m_window(window) {
	// create member object
	m_display_list = std::shared_ptr<ThreadSafeVector<DisplayListContent>>
		(new ThreadSafeVector<DisplayListContent>(std::vector<DisplayListContent>())); 

	m_preview_bitmaps = std::shared_ptr<ThreadSafeVector<CachedBitmap>>
		(new ThreadSafeVector<CachedBitmap>(std::vector<CachedBitmap>()));
	
	m_cachedBitmaps = std::shared_ptr<ThreadSafeDeque<CachedBitmap>>
		(new ThreadSafeDeque<CachedBitmap>(std::deque<CachedBitmap>())); 

	m_render_queue = std::shared_ptr<ThreadSafeDeque<std::shared_ptr<RenderInfo>>>
		(new ThreadSafeDeque<std::shared_ptr<RenderInfo>>(std::deque<std::shared_ptr<RenderInfo>>())); 

	m_display_list_thread = std::thread([this] { create_display_list(); });
	m_preview_bitmap_thread = std::thread([this] { create_preview(0.1f); });

	for (size_t i = 0; i < amount_threads; i++) {
		m_render_threads.push_back(std::thread([this] { async_render(); }));
	}
}

void PDFRenderHandler::create_preview(float scale) {
	Logger::log(L"Start creating Preview Bitmaps");
	{
		auto prev = m_preview_bitmaps->get_write();
		prev->clear();
	}

	auto dpi = m_renderer->get_dpi();

	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		{
			auto dest = m_pdf->get_pagerec()->get_read();
			CachedBitmap p; 
			p.bitmap = get_bitmap(*m_renderer, i, dpi * scale);
			p.doc_coords.width = dest->at(i).width;
			p.doc_coords.height = dest->at(i).height;
			p.page = i; 
			p.dpi = dpi;
			{ 
				auto prev = m_preview_bitmaps->get_write(); 
				prev->push_back(std::move(p));
			}
		}
		m_window->invalidate_drawing_area(); 
	}

	m_preview_scale = scale;
	m_preview_bitmaps_processed = true;
	Logger::log(L"Finished creating Preview Bitmaps");
}

template<typename T>
void remove_small_rects(std::vector <Renderer::Rectangle<T>>& rects, float threshold = EPSILON) {
	for (long i = static_cast<long>(rects.size() - 1); i >= 0; i--) {
		rects.at(i).validate();
		if (rects.at(i).width < threshold or rects.at(i).height < threshold) {
			rects.erase(rects.begin() + i);
		}
	}
}

std::vector<size_t> PDFRenderHandler::render_job_clipped_cashed_bitmaps(Renderer::Rectangle<float> overlap_clip_space, float dpi) {
	auto cached_bitmap = m_cachedBitmaps->get_read();
	auto page_rec = m_pdf->get_pagerec()->get_read();

	std::vector<size_t> return_value;
	for (size_t i = 0; i < cached_bitmap->size(); i++) {
		// check if the dpi is high enough
		if (cached_bitmap->at(i).dpi < dpi and FLOAT_EQUAL(cached_bitmap->at(i).dpi, dpi) == false)
			continue;
		if (overlap_clip_space.intersects(page_rec->at(cached_bitmap->at(i).page)) == false)
			continue;

		return_value.push_back(i);
	}
	return return_value;
}

std::vector<Renderer::Rectangle<float>> PDFRenderHandler::render_job_splice_recs(Renderer::Rectangle<float> overlap, const std::vector<size_t>& cashedBitmapindex) {
	std::vector<Renderer::Rectangle<float>> other;	

	auto cached_bitmaps = m_cachedBitmaps->get_read();
	auto page_rec = m_pdf->get_pagerec()->get_read(); 

	other.reserve(cashedBitmapindex.size());
	for (size_t i = 0; i < cashedBitmapindex.size(); i++) {
		other.push_back(page_rec->at(cached_bitmaps->at(cashedBitmapindex.at(i)).page)); 
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
	auto queue = m_render_queue->get_write();
	for (long i = static_cast<long>(queue->size() - 1); i >= 0; i--) {
		if (queue->at(i)->stat == RenderInfo::STATUS::FINISHED or queue->at(i)->stat == RenderInfo::STATUS::ABORTED)
			queue->erase(queue->begin() + i); 
	}
}

void PDFRenderHandler::render_high_res() {
	render_high_res(RenderInstructions());
}

void PDFRenderHandler::render_high_res(RenderInstructions instruct) {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size_normalized());

	std::vector<std::tuple<size_t, Renderer::Rectangle<float>, std::vector<CachedBitmap*>>> clipped_documents;

	reduce_cache_size(200);

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
		auto dest = m_pdf->get_pagerec()->get_read();
		if (dest->at(i).intersects(clip_space)) {
			// this is the overlap in the clip space
			auto overlap = clip_space.calculate_overlap(dest->at(i)); 

			//Check if page overlaps with any chached bitmap
			std::vector<size_t> cached_bitmap_index = render_job_clipped_cashed_bitmaps(overlap, dpi);

			// check if the bitmaps are already queued
			std::vector<Renderer::Rectangle<float>> queued_and_cached_bitmaps;
			auto cached_bitmaps = m_cachedBitmaps->get_write();
			auto render_queue = m_render_queue->get_write();

			queued_and_cached_bitmaps.reserve(render_queue->size());
			for (size_t j = 0; j < render_queue->size(); j++) {
				auto docspace = render_queue->at(j)->overlap_in_docspace;
				docspace += dest->at(render_queue->at(j)->page).upperleft();
				queued_and_cached_bitmaps.push_back(docspace);
			}

			for (size_t j = 0; j < cached_bitmap_index.size(); j++) { 
				queued_and_cached_bitmaps.push_back(cached_bitmaps->at(cached_bitmap_index.at(j)).doc_coords + dest->at(cached_bitmaps->at(cached_bitmap_index.at(j)).page).upperleft());
			}

			// if there are no cached bitmaps we have to render everything
			if (queued_and_cached_bitmaps.empty()) {
				// add stitch
				lambda_add_stitch(overlap);

				// need to transfom into doc space
				overlap -= dest->at(i).upperleft();

				// add to the queue
				render_queue->push_back(std::shared_ptr<RenderInfo>(new RenderInfo({ i, dpi, overlap, instruct })));

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
				chopped_render_targets.at(j) -= dest->at(i).upperleft();

				// make choppy bigger so the are no stitch lines
				lambda_add_stitch(chopped_render_targets.at(j));

				// add the chopped up recs to the queue
				render_queue->push_back(std::shared_ptr<RenderInfo>(new RenderInfo({ i, dpi, chopped_render_targets.at(j), instruct }))); 
			}

			// add all bitmaps into an array so it can be send to the main window
			std::vector<CachedBitmap*> temp_cachedBitmaps;
			for (size_t i = 0; i < cached_bitmap_index.size(); i++) {
				temp_cachedBitmaps.push_back(&cached_bitmaps->at(cached_bitmap_index.at(i)));
			}
			// we can send some of the cached bitmaps already to the window
			// TODO change so it takes const chachedbitmaps
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
	auto dest		    = m_pdf->get_pagerec()->get_read(); 
	auto cached_bitmaps = m_cachedBitmaps->get_write();
	auto render_queue   = m_render_queue->get_write();
	std::vector<CachedBitmap*> temp_cachedBitmaps; 
	for (size_t i = 0; i < render_queue->size(); i++) {
		auto page = render_queue->at(i)->page;
		auto overlap = render_queue->at(i)->overlap_in_docspace;
		auto butmap = get_bitmap(*m_renderer, page, overlap, dpi); 
	
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
	auto page_rec     = m_pdf->get_pagerec()->get_read();
	auto render_queue = m_render_queue->get_write();
	auto dpi		  = m_renderer->get_dpi() * m_renderer->get_transform_scale();

	for (long i = static_cast<long>(render_queue->size() - 1); i >= 0; i--) {
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
	struct ClonedDisplayList {
		fz_display_list* content = nullptr;
		fz_display_list* annots = nullptr;
		fz_display_list* widgets = nullptr;
	};


	std::vector<ClonedDisplayList> cloned_lists;
	{
		auto list_array = m_display_list->get_read();
		for (size_t i = 0; i < list_array->size(); i++) {
			// retrieve list
			auto list = list_array->at(i).m_page_annots;  

			// do the copy operation
			auto copy_list = [ctx](fz_display_list* list) { 
				fz_display_list* ls = fz_new_display_list(ctx, (list)->mediabox); 
				ls->len = (list)->len;
				ls->max = (list)->max;
				ls->list = new fz_display_node[ls->max];
				memcpy(ls->list, (list)->list, (list)->len * sizeof(fz_display_node));
				return ls;
			};

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
			// the order here is very important since we want to render the content first and then the annots on top of it
			if (info->instructions.render_content) { 
				fz_run_display_list(ctx, cloned_lists.at(info->page).content, drawdevice, fz_identity, info->overlap_in_docspace, info->cookie);
			}
			if (info->instructions.render_annots) {
				fz_run_display_list(ctx, cloned_lists.at(info->page).annots, drawdevice, fz_identity, info->overlap_in_docspace, info->cookie); 
			}
			if (info->instructions.render_widgets) {
				fz_run_display_list(ctx, cloned_lists.at(info->page).widgets, drawdevice, fz_identity, info->overlap_in_docspace, info->cookie); 
			}
			
			// check if the rendering was aborted
			if (info->cookie->abort == 1) {
				info->stat = RenderInfo::STATUS::ABORTED; 
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

			auto bitmaps_list = m_cachedBitmaps->get_write();
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
		auto delete_lambda = [ctx](fz_display_list* list) {
			delete[] list->list;
			list->list = nullptr;
			list->len = 0;
			list->max = 0;
			fz_drop_display_list(ctx, list);
		};
		delete_lambda(cloned_lists.at(i).content);
		delete_lambda(cloned_lists.at(i).annots);
		delete_lambda(cloned_lists.at(i).widgets);
	}

	fz_drop_context(ctx);
	--m_amount_thread_running;
	m_render_queue_condition_var.notify_all(); 
}

float PDFRenderHandler::get_display_list_progress() {
	return (float)((double)m_display_list_amount_processed / (double)m_display_list_amount_processed_total); 
}

void PDFRenderHandler::remove_small_cached_bitmaps(float treshold) {
	auto cached_bitmaps = m_cachedBitmaps->get_write();
	if (cached_bitmaps->size() == 0)
		return;

	for (long i = static_cast<long>(cached_bitmaps->size() - 1); i >= 0; i--) {
		if ((cached_bitmaps->at(i).doc_coords.width < treshold or cached_bitmaps->at(i).doc_coords.height < treshold) and cached_bitmaps->at(i).used == 0) {
			cached_bitmaps->erase(cached_bitmaps->begin() + i);
		}
	}
}

void PDFRenderHandler::reduce_cache_size(unsigned long long max_memory) {
	auto cached_bitmaps = m_cachedBitmaps->get_write();

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
	// High res rendering
	m_display_list = std::move(t.m_display_list);
	m_display_list_thread = std::move(t.m_display_list_thread);
	m_cachedBitmaps = std::move(t.m_cachedBitmaps);
}

PDFRenderHandler& PDFRenderHandler::operator=(PDFRenderHandler&& t) noexcept {
	// new c++ stuff?
	this->~PDFRenderHandler();
	new(this) PDFRenderHandler(std::move(t)); 
	return *this;
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

		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp));
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

void PDFRenderHandler::render() {
	render(RenderInstructions());
}

void PDFRenderHandler::render(RenderInstructions instruct) {
	auto scale = m_renderer->get_transform_scale();

	if (instruct.render_outline) {
		render_outline();
	}

	if (instruct.render_preview) {
		render_preview();

		auto cached = m_cachedBitmaps->get_write();
		// add all bitmaps into an array so it can be send to the main window
		std::vector<CachedBitmap*> temp_cachedBitmaps;
		for (size_t i = 0; i < cached->size(); i++) {
			temp_cachedBitmaps.push_back(&cached->at(i));
		}
		// we can send some of the cached bitmaps already to the window
		SendMessage(m_window->get_hwnd(), WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));
	}

	if (scale > m_preview_scale + EPSILON and m_preview_bitmaps_processed and instruct.render_highres) {
		// if we are zoomed in, we need to render the page at a higher resolution
		// than the screen resolution
		render_high_res(instruct); 
	}
}

void PDFRenderHandler::clear_render_cache() {
	auto cached = m_cachedBitmaps->get_write();
	cached->clear();
}

unsigned long long PDFRenderHandler::get_cache_memory_usage() const {
	auto cached_bitmaps = m_cachedBitmaps->get_write();

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
