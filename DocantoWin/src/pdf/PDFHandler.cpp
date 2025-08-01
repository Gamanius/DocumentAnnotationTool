#include "PDFHandler.h"

using namespace Docanto;

DocantoWin::PDFHandler::PDFHandler(const std::filesystem::path& p, std::shared_ptr<Direct2DRender> render) : m_render(render) {
	m_pdfimageprocessor = std::make_shared<PDFHandlerImageProcessor>(render);
	auto pdf = std::make_shared<PDF>(p);
	auto r = std::make_shared<PDFRenderer>(pdf, m_pdfimageprocessor);
	auto a = std::make_shared<PDFAnnotation>(pdf);
	m_pdfobj.push_back({ pdf, r, a });

	m_pdfobj.back().render->set_rendercallback([&](size_t i) {
		PostMessage(m_render->get_attached_window()->get_hwnd(), WM_PAINT, 0, 0);
		});
}

void DocantoWin::PDFHandler::add_pdf(const std::filesystem::path& p) {
	auto pdf = std::make_shared<PDF>(p);
	auto r = std::make_shared<PDFRenderer>(pdf, m_pdfimageprocessor);
	auto a = std::make_shared<PDFAnnotation>(pdf);
	m_pdfobj.push_back({ pdf, r, a});
	m_pdfobj.back().render->set_rendercallback([&](size_t i) {
		PostMessage(m_render->get_attached_window()->get_hwnd(), WM_PAINT, 0, 0);
	});

	auto max_width = 0;
	for (size_t i = 0; i < m_pdfobj.size() - 1; i++) {
		max_width += m_pdfobj.at(i).render->get_max_dimension().width + 20;
	}

	for (size_t i = 0; i < m_pdfobj.back().pdf->get_page_count(); i++) {
		auto r = m_pdfobj.back().render;
		auto last_pos = r->get_position(i);
		r->set_position(i, { last_pos.x + max_width, last_pos.y });
	}
}

void DocantoWin::PDFHandler::draw() {
	for (auto& obj : m_pdfobj) {
		draw(obj.render);
	}
}

void DocantoWin::PDFHandler::draw(std::shared_ptr<Docanto::PDFRenderer> r) {
	auto recs = r->get_clipped_page_recs();
	for (const auto& r : recs) {
		m_render->draw_rect_filled(r, {255, 255, 255});
	}

	auto& preview_info = r->get_preview();
	auto bitmaps = m_pdfimageprocessor->m_all_bitmaps.get();

	for (const auto& info : preview_info) {
		m_render->draw_bitmap(info.recs, bitmaps->at(info.id));
	}

	auto list = r->draw();
	auto& highdef = *list;

	for (const auto& info : highdef) {
		if (bitmaps->find(info.id) == bitmaps->end()) {
			Docanto::Logger::warn("Couldnt find ", info.id);
			continue;
		}
		if (bitmaps->at(info.id).m_object == nullptr) {
			continue;
		}
		m_render->draw_bitmap(info.recs + r->get_position(info.page), bitmaps->at(info.id));
	}

	auto annot_list = r->annot();
	auto& anottation = *annot_list;

	for (const auto& info : anottation) {
		if (bitmaps->find(info.id) == bitmaps->end()) {
			Docanto::Logger::warn("Couldnt find ", info.id);
			continue;
		}
		if (bitmaps->at(info.id).m_object == nullptr) {
			continue;
		}
		m_render->draw_bitmap(info.recs + r->get_position(info.page), bitmaps->at(info.id));
	}

	if (m_debug_draw) r->debug_draw(m_render);
}

void DocantoWin::PDFHandler::set_debug_draw(bool b) {
	m_debug_draw = b;
}

void DocantoWin::PDFHandler::toggle_debug_draw() {
	set_debug_draw(!m_debug_draw);
}

std::pair<DocantoWin::PDFHandler::PDFWrapper, size_t> DocantoWin::PDFHandler::get_pdf_at_point(Docanto::Geometry::Point<float> p) {
	for (auto& obj : m_pdfobj) {
		auto all_recs = obj.render->get_page_recs();

		for (size_t i = 0; i < all_recs.size(); i++) {
			auto transformed_rect = m_render->transform(all_recs[i]);
			if (transformed_rect.intersects(p)) {
				return  { obj, i };
			}
		}
	}
	return {{} , ~static_cast<size_t>(0)};
}

void DocantoWin::PDFHandler::reload() {
	for (auto& pdf : m_pdfobj) {
		pdf.render->reload();
	}

	m_render->get_attached_window()->send_paint_request();
}

size_t DocantoWin::PDFHandler::get_amount_of_pdfs() const {
	return m_pdfobj.size();
}

void DocantoWin::PDFHandler::request() {
	auto scale = WINDOWS_DEFAULT_DPI / m_render->get_dpi();
	auto dims = m_render->get_attached_window()->get_client_size();
	auto local_rec =  m_render->inv_transform({ 0, 0, (float)dims.width * scale, (float)dims.height * scale});


	for (auto& obj : m_pdfobj) {
		obj.render->request(local_rec, m_render->get_transform_scale() * m_render->get_dpi());
	}
}

DocantoWin::PDFHandler::PDFHandlerImageProcessor::PDFHandlerImageProcessor(std::shared_ptr<Direct2DRender> render) {
	m_render = render;
}

void DocantoWin::PDFHandler::PDFHandlerImageProcessor::processImage(size_t id, const Docanto::Image& img) {
	auto bitmaps = m_all_bitmaps.get();
	bitmaps->operator[](id) = m_render->create_bitmap(img);
}

void DocantoWin::PDFHandler::PDFHandlerImageProcessor::deleteImage(size_t id) {
	auto bitmaps = m_all_bitmaps.get();
	if (bitmaps->find(id) != bitmaps->end()) {
		bitmaps->erase(id);
	}
}
