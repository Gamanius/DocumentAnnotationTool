#include "include.h"

void PDFHandler::create_preview(float scale) {
	m_previewBitmaps.clear();

	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		m_previewBitmaps.push_back(m_pdf.get_bitmap(*m_renderer, i, m_renderer->get_dpi() * scale));
	}

	m_preview_scale = scale;
}

void multithreaded_high_res(PDFHandler* pdfhandler, Direct2DRenderer* const renderer, MuPDFHandler::PDF* pdf, size_t page, Renderer::Rectangle<float> source, Renderer::Rectangle<float> pagerec) {
	// render page
	
	//auto butmap = pdf->multithreaded_get_bitmap(renderer, page, source, renderer->get_dpi() * renderer->get_transform_scale()); 
	
	source.x += pagerec.x;
	source.y += pagerec.y;

	pdfhandler->draw_lock();
	renderer->begin_draw();
	//renderer->draw_bitmap(butmap, source, Direct2DRenderer::INTERPOLATION_MODE::LINEAR);
	renderer->end_draw();
	pdfhandler->draw_unlock();
}

void PDFHandler::render_high_res() {
	// first check which pages intersect with the clip space
	auto clip_space = m_renderer->inv_transform_rect(m_renderer->get_window_size());
	std::vector<std::tuple<size_t, Renderer::Rectangle<float>>> clipped_documents;
	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		// first check if they intersect
		if (m_pagerec.at(i).intersects(clip_space)) {
			// this is the overlap in the clip space
			auto overlap = clip_space.calculate_overlap(m_pagerec.at(i)); 
			// transform into doc space
			overlap.x -= m_pagerec.at(i).x;
			overlap.y -= m_pagerec.at(i).y;
			// add it to the pile
			clipped_documents.push_back(std::make_tuple(i, overlap));
		}
	}

	// NON multithreaded solution
	//for (size_t i = 0; i < clipped_documents.size(); i++) {
	//	auto page = std::get<0>(clipped_documents.at(i));
	//	auto overlap = std::get<1>(clipped_documents.at(i));
	//	auto butmap = m_pdf.get_bitmap(*m_renderer, page, overlap, m_renderer->get_dpi() * m_renderer->get_transform_scale());
	//	m_renderer->begin_draw(); 
	//	// transform from doc space to clip space
	//	overlap.x += m_pagerec.at(page).x;
	//	overlap.y += m_pagerec.at(page).y;
	//
	//	m_renderer->draw_bitmap(butmap, overlap, Direct2DRenderer::INTERPOLATION_MODE::LINEAR); 
	//	m_renderer->end_draw();
	//}

	// run through all documents and create threads
	for (size_t i = 0; i < clipped_documents.size(); i++) {
		// divide rec into 4 equal parts
		auto page        = std::get<0>(clipped_documents.at(i));
		auto overlap     = std::get<1>(clipped_documents.at(i));

		float halfwidth  = overlap.width / 2.0f;
		float halfheight = overlap.height/ 2.0f;

		// create the 4 recs
		Renderer::Rectangle<float> r1 = { overlap.x, overlap.y, halfwidth, halfheight };
		Renderer::Rectangle<float> r2 = { overlap.x + halfwidth, overlap.y, halfwidth, halfheight };
		Renderer::Rectangle<float> r3 = { overlap.x, overlap.y + halfheight, halfwidth, halfheight };
		Renderer::Rectangle<float> r4 = { overlap.x + halfwidth, overlap.y + halfheight, halfwidth, halfheight };

		// create 4 display lists
		fz_display_list* list1 = nullptr;
		fz_display_list* list2 = nullptr;
		fz_display_list* list3 = nullptr;
		fz_display_list* list4 = nullptr;
		fz_try(m_pdf.m_ctx) {
			// create display list
			list1 = fz_new_display_list(m_pdf.m_ctx, r1);
			list2 = fz_new_display_list(m_pdf.m_ctx, r2);
			list3 = fz_new_display_list(m_pdf.m_ctx, r3);
			list4 = fz_new_display_list(m_pdf.m_ctx, r4);

			auto dev1 = fz_new_list_device(m_pdf.m_ctx, list1);
			auto dev2 = fz_new_list_device(m_pdf.m_ctx, list2);
			auto dev3 = fz_new_list_device(m_pdf.m_ctx, list3);
			auto dev4 = fz_new_list_device(m_pdf.m_ctx, list4);

			fz_run_page(m_pdf.m_ctx, m_pdf.get_page(page), dev1, fz_identity, NULL);
			fz_run_page(m_pdf.m_ctx, m_pdf.get_page(page), dev2, fz_identity, NULL);
			fz_run_page(m_pdf.m_ctx, m_pdf.get_page(page), dev3, fz_identity, NULL);
			fz_run_page(m_pdf.m_ctx, m_pdf.get_page(page), dev4, fz_identity, NULL);

			// close device and flush
			fz_close_device(m_pdf.m_ctx, dev1);
			fz_close_device(m_pdf.m_ctx, dev2);
			fz_close_device(m_pdf.m_ctx, dev3);
			fz_close_device(m_pdf.m_ctx, dev4);
		} fz_catch(m_pdf.m_ctx) {
			fz_rethrow(m_pdf.m_ctx);
		}

		// create 4 threads
		std::thread t1(multithreaded_high_res, this, m_renderer, &m_pdf, page, r1, m_pagerec.at(page));
		std::thread t2(multithreaded_high_res, this, m_renderer, &m_pdf, page, r2, m_pagerec.at(page));
		std::thread t3(multithreaded_high_res, this, m_renderer, &m_pdf, page, r3, m_pagerec.at(page));
		std::thread t4(multithreaded_high_res, this, m_renderer, &m_pdf, page, r4, m_pagerec.at(page));

		t1.join();
		t2.join();
		t3.join();
		t4.join();
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
	Logger::log("Preview");
	Logger::print_to_debug();
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

void PDFHandler::render() {
	auto scale = m_renderer->get_transform_scale();
	//render_preview();

	if (scale > m_preview_scale + EPSILON) {
		// if we are zoomed in, we need to render the page at a higher resolution
		// than the screen resolution
		render_high_res();
	}
	else {
		render_preview();
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

void PDFHandler::draw_lock() {
	m_draw_lock.lock();
}

void PDFHandler::draw_unlock() {
	m_draw_lock.unlock();
}

PDFHandler::~PDFHandler() {
	//m_pdf will be destroyed automatically
	//m_preview will be destroyed automatically
}
