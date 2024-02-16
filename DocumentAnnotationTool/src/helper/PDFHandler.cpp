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

void do_multihread_rendering(fz_context* ctx, std::shared_ptr<std::condition_variable> cv, std::shared_ptr<std::mutex> m,
	std::shared_ptr<std::map<size_t, PDFRenderHandler::RenderInfo>> render_queue,
	std::shared_ptr<bool> die, std::shared_ptr<std::vector<fz_display_list*>> displaylist, std::shared_ptr<std::deque<CachedBitmap>> globalcachedbitmaps,
	std::shared_ptr<std::atomic_size_t> amount_of_threads, std::shared_ptr<std::atomic_size_t> renderinprogress) {

	fz_context* cloned_ctx = fz_clone_context(ctx);

	// Copy all lists
	// this is most likely not safe AT ALL
	std::vector<fz_display_list*> cloned_lists;
	for (size_t i = 0; i < displaylist->size(); i++) {
		fz_display_list* ls = fz_new_display_list(cloned_ctx, displaylist->at(i)->mediabox);
		ls->len = displaylist->at(i)->len;
		ls->max = displaylist->at(i)->max;
		ls->list = new fz_display_node[ls->max];
		memcpy(ls->list, displaylist->at(i)->list, displaylist->at(i)->len * sizeof(fz_display_node));

		cloned_lists.push_back(ls);
	}

	while (!(*die)) {
		// lock to get access to the deque and wait for the conditional variable
		std::unique_lock<std::mutex> lock(*m);
		cv->wait(lock, [&] { return render_queue->size() != 0; });
		// first check if cleanup is neccesesary
		if (*die) {
			break;
		}

		// If notified remove the first element and render it
		std::vector<CachedBitmap*> cached_bitmaps;
		size_t map_index = 0; 
		bool found = false;

		// safety check that is not needed
		if (render_queue->empty()) {
			lock.unlock();
			continue;
		}

		// get the first element that is not currently being rendererd
		for (auto& e : *render_queue) {
			if (e.second.render_status != 0) {
				continue;
			}

			map_index = e.first;
			found = true;
			break;
		}

		// if we didnt find anything we can continue
		if (found == false) {
			lock.unlock();
			continue;
		}
		
		// update the map
		(*renderinprogress)++;
		render_queue->at(map_index).render_status++;
		PDFRenderHandler::RenderInfo& pdfrenderinfo = render_queue->at(map_index);

		// we can unlock now since we maybe got the element and copied it
		lock.unlock();

		fz_matrix ctm;
		ctm = fz_scale((pdfrenderinfo.dpi / MUPDF_DEFAULT_DPI), (pdfrenderinfo.dpi / MUPDF_DEFAULT_DPI));
		auto transform = fz_translate(-pdfrenderinfo.source.x, -pdfrenderinfo.source.y);
		// this is to calculate the size of the pixmap using the source
		auto pixmap_size = fz_round_rect(fz_transform_rect(fz_make_rect(0, 0, pdfrenderinfo.source.width, pdfrenderinfo.source.height), ctm));

		fz_pixmap* pixmap = nullptr;
		fz_device* drawdevice = nullptr;
		Direct2DRenderer::BitmapObject obj;

		fz_try(cloned_ctx) {
			// create new pixmap
			pixmap = fz_new_pixmap_with_bbox(cloned_ctx, fz_device_rgb(cloned_ctx), pixmap_size, nullptr, 1);
			fz_clear_pixmap_with_value(cloned_ctx, pixmap, 0xff); // for the white background
			// create draw device
			drawdevice = fz_new_draw_device(cloned_ctx, fz_concat(transform, ctm), pixmap);
			// render to draw device
			fz_run_display_list(cloned_ctx, cloned_lists.at(pdfrenderinfo.page), drawdevice, fz_identity, pdfrenderinfo.source, &pdfrenderinfo.pdf_cookie);
			fz_close_device(cloned_ctx, drawdevice);
			// create the bitmap
			obj = pdfrenderinfo.renderer->create_bitmap((byte*)pixmap->samples, pixmap_size, (unsigned int)pixmap->stride, pdfrenderinfo.dpi); // default dpi of the pixmap

			// we can call the window and lock the mutex since we are modifying stuff again
			lock.lock();
			auto new_cachedbitmap = CachedBitmap(std::move(obj), pdfrenderinfo.dest, pdfrenderinfo.dpi, 1);
			cached_bitmaps.push_back(&new_cachedbitmap);
			// unlock to avoid deadlock (If this thread got the lock first and the main thread is waiting on the lock, not unlocking will
			// lead to a deadlock)
			lock.unlock();
			SendMessage(pdfrenderinfo.callback_window, WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&cached_bitmaps));
			lock.lock();
			// we can remove the item from the queue
			render_queue->erase(map_index);  
			// we can now decreament the used counter
			for (size_t i = 0; i < cached_bitmaps.size(); i++) {
				if (cached_bitmaps.at(i)->used != 0)
					cached_bitmaps.at(i)->used--;
			}
			// now add the new cached bitmap to the other ones
			globalcachedbitmaps->push_back(std::move(new_cachedbitmap));
			lock.unlock();
		} fz_always(cloned_ctx) {
			// drop all devices
			fz_drop_device(cloned_ctx, drawdevice);
			fz_drop_pixmap(cloned_ctx, pixmap);
		} fz_catch(cloned_ctx) {
			return;
		}
		(*renderinprogress)--;
	}
	// delete all the list elements
	for (size_t i = 0; i < cloned_lists.size(); i++) {
		// also not safe and probably not intended
		delete[] cloned_lists.at(i)->list;
		cloned_lists.at(i)->list = nullptr;
		cloned_lists.at(i)->len = 0;
		cloned_lists.at(i)->max = 0;
		fz_drop_display_list(cloned_ctx, cloned_lists.at(i));
	}
	fz_drop_context(cloned_ctx);
	(*amount_of_threads)--;
	cv->notify_all();
}


void PDFRenderHandler::create_render_threads(size_t amount) {
	*(m_amount_of_threads_running) += amount;
	for (size_t i = 0; i < amount; i++) {
		std::thread t(do_multihread_rendering, (fz_context*)*m_pdf, m_thread_conditional,  
			m_queue_mutex, m_render_queue, m_thread_should_die, m_display_lists, 
			m_cachedBitmaps, m_amount_of_threads_running, m_render_in_progress);
		t.detach();
	}
}


PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer) : m_pdf(pdf), m_renderer(renderer) {
	m_cachedBitmaps = std::make_shared<std::deque<CachedBitmap>>(); 
	m_queue_mutex = std::make_shared<std::mutex>();
	m_amount_of_threads_running = std::make_shared<std::atomic_size_t>(0);
	m_render_in_progress = std::make_shared<std::atomic_size_t>(0);
	m_thread_conditional = std::make_shared<std::condition_variable>();
	m_thread_should_die = std::make_shared<bool>(false); 
	m_render_queue = std::make_shared<std::map<size_t, RenderInfo>>();


	// init of m_pagerec
	sort_page_positions();
	// init of m_previewBitmaps
	create_preview(1); // create preview with default scale

	create_display_list();
}

PDFRenderHandler::PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, size_t amount_of_render) : PDFRenderHandler(pdf, renderer) {
	create_render_threads(amount_of_render);
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


void PDFRenderHandler::update_render_queue() {
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	std::vector<size_t> indices;
	for (auto& e : *m_render_queue) {
		// check if the clipspace is still overlapping with the source
		if (e.second.dest.intersects(clip_space)) {
			// do nothing
			continue;
		}

		if (e.second.render_status != 0) {
			// abort the rendering
			e.second.pdf_cookie.abort = 1; 
		}
		else {
			// we can safely remove the element
			indices.push_back(e.first);
		}
	}

	for (size_t i = 0; i < indices.size(); i++) {
		m_render_queue->erase(indices.at(i));
	}
}

void PDFRenderHandler::render_high_res(HWND window) {
	// get the target dpi
	auto dpi = m_renderer->get_dpi() * m_renderer->get_transform_scale();
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	std::vector<std::tuple<size_t, Renderer::Rectangle<float>, std::vector<CachedBitmap*>>> clipped_documents;

	std::vector<CachedBitmap*> temp_cachedBitmaps;

	///////// ACCESS TO CACHED BITMAPS //////////
	std::unique_lock<std::mutex> lock(*(m_queue_mutex));

	remove_small_cached_bitmaps(50 / m_renderer->get_transform_scale());
	if (m_cachedBitmaps->size() > 50) {
		size_t offset = 0;
		for (size_t i = 0; i < m_cachedBitmaps->size(); i++) {
			if (m_cachedBitmaps->at(i).used == 0) {
				m_cachedBitmaps->erase(m_cachedBitmaps->begin() + i - offset);
				offset++;
			}
			if (m_cachedBitmaps->size() < 50) {
				break;
			}
		}
	}

	// TODO
	//update_render_queue();
	
	for (size_t i = 0; i < m_pdf->get_page_count(); i++) {
		// clear the cached bitmap vector
		temp_cachedBitmaps.clear();
		// first check if they intersect
		if (m_pagerec.at(i).intersects(clip_space)) {
			// this is the overlap in the clip space
			auto overlap = clip_space.calculate_overlap(m_pagerec.at(i)); 

			// add all bitmaps sizes of the bitmaps with high enough dpi into an array
			std::vector<Renderer::Rectangle<float>> bitmap_dest;

			bitmap_dest.reserve(m_cachedBitmaps->size());
			for (auto& j : *m_cachedBitmaps) {
				// check if the dpi is high enough
				if (FLOAT_EQUAL(j.dpi, dpi) == false or overlap.intersects(j.dest) == false)
					continue;

				bitmap_dest.push_back(j.dest);

				// add all the cached bitmaps to the temp vector
				temp_cachedBitmaps.push_back(&j);
			}

			// if there are no cached bitmaps we have to render everything
			if (bitmap_dest.empty()) {
				overlap.x -= PDF_STITCH_THRESHOLD;
				overlap.y -= PDF_STITCH_THRESHOLD;
				overlap.width  += PDF_STITCH_THRESHOLD * 2;
				overlap.height += PDF_STITCH_THRESHOLD * 2;

				// need to transfom into doc space
				overlap.x -= m_pagerec.at(i).x;
				overlap.y -= m_pagerec.at(i).y;

				clipped_documents.push_back(std::make_tuple(i, overlap, std::vector<CachedBitmap*>()));
				continue;
			}

			// now we can chop up the render target. These are all the rectangles that we need to render
			std::vector<Renderer::Rectangle<float>> choppy = splice_rect(overlap, bitmap_dest);
			merge_rects(choppy); 
			remove_small_rects(choppy);
			//Logger::log("Choppy size: " + std::to_string(choppy.size()) + "\n");
			//Logger::print_to_debug();

			/*
			overlap.x -= m_pagerec.at(i).x;
			overlap.y -= m_pagerec.at(i).y;
			*/
			

			// increase the used counter
			for (size_t j = 0; j < temp_cachedBitmaps.size(); j++) {
				temp_cachedBitmaps.at(j)->used += choppy.size();
			}

			for (size_t j = 0; j < choppy.size(); j++) {
				// just in case
				choppy.at(j).validate();
				// transform into doc space
				choppy.at(j).x -= m_pagerec.at(i).x;
				choppy.at(j).y -= m_pagerec.at(i).y;

				// make choppy bigger so the are no stitch lines
				choppy.at(j).x -= PDF_STITCH_THRESHOLD;  /* this is just a magic number */
				choppy.at(j).y -= PDF_STITCH_THRESHOLD; 
				choppy.at(j).width  += PDF_STITCH_THRESHOLD * 2;
				choppy.at(j).height += PDF_STITCH_THRESHOLD * 2;

				// and push back
				clipped_documents.push_back(std::make_tuple(i, choppy.at(j), temp_cachedBitmaps));
			}

			// we can send some of the cached bitmaps already to the window
			SendMessage(window, WM_PDF_BITMAP_READY, (WPARAM)nullptr, (LPARAM)(&temp_cachedBitmaps));

			// add it to the pile
			//clipped_documents.push_back(std::make_tuple(i, overlap));
		}
	}

	//// NON multithreaded solution
	//for (size_t i = 0; i < clipped_documents.size(); i++) {
	//	auto page = std::get<0>(clipped_documents.at(i));
	//	auto overlap = std::get<1>(clipped_documents.at(i)); 
	//	auto butmap = get_bitmap(*m_renderer, page, overlap, dpi); 
	//	// transform from doc space to clip space
	//	overlap.x += m_pagerec.at(page).x;
	//	overlap.y += m_pagerec.at(page).y;
	//
	//	m_renderer->begin_draw();  
	//	m_renderer->set_current_transform_active();
	//	m_renderer->draw_bitmap(butmap, overlap, Direct2DRenderer::INTERPOLATION_MODE::LINEAR);  
	//	m_renderer->end_draw(); 
	//	
	//	m_cachedBitmaps->push_back({std::move(butmap), overlap, dpi, 0 });
	//}

	//run through all documents and create threads
	for (size_t i = 0; i < clipped_documents.size(); i++) {
		RenderInfo info;
		info.page        = std::get<0>(clipped_documents.at(i)); 
		auto overlap     = std::get<1>(clipped_documents.at(i)); 
		auto cached      = std::get<2>(clipped_documents.at(i));  
		auto dest		 = overlap; 
	
		dest.x += m_pagerec.at(info.page).x; 
		dest.y += m_pagerec.at(info.page).y; 
	
		info.dest = dest; 
		info.renderer = m_renderer; 
		info.dpi = dpi; 
		info.source = overlap; 
		info.callback_window = window;
	
		// add it to the queueu
		m_render_queue->insert(std::make_pair(m_render_queue_frame, info));
		m_render_queue_frame++;
	}
	
	m_thread_conditional->notify_all();
}

void PDFRenderHandler::remove_small_cached_bitmaps(float treshold) {
	for (int i = m_cachedBitmaps->size() - 1; i >= 0; i--) {
		if ((m_cachedBitmaps->at(i).dest.width < treshold or m_cachedBitmaps->at(i).dest.height < treshold) and m_cachedBitmaps->at(i).used == 0) {
			m_cachedBitmaps->erase(m_cachedBitmaps->begin() + i);
		}
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

	// move operator for the multithreading
	m_queue_mutex               = std::move(t.m_queue_mutex);
	m_amount_of_threads_running = std::move(t.m_amount_of_threads_running);
	m_render_in_progress        = std::move(t.m_render_in_progress);
	m_thread_conditional        = std::move(t.m_thread_conditional);
	m_thread_should_die         = std::move(t.m_thread_should_die);
	m_render_queue_frame        = t.m_render_queue_frame;
	m_render_queue              = std::move(t.m_render_queue);

	t.m_render_queue_frame = 0;
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

void PDFRenderHandler::stop_render_threads() {
	*m_thread_should_die = true;
	// check how many threads are alive
	if (*m_amount_of_threads_running == 0) {
		return;
	}
	// put dummy data in the queue
	m_render_queue->insert(std::make_pair(0, RenderInfo()));
	// notify all threads 
	m_thread_conditional->notify_all();

	MSG msg;
	while (*m_render_in_progress != 0) 
		GetMessage(&msg, 0, 0, 0);

	std::unique_lock<std::mutex> lock(*m_queue_mutex);
	m_thread_conditional->wait(lock, [this] { return *m_amount_of_threads_running == 0; });
}

PDFRenderHandler::~PDFRenderHandler() {
	// Need to clean up the threads
	if (m_amount_of_threads_running != nullptr)
		stop_render_threads();
}
