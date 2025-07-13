#include "PDFHandler.h"

using namespace Docanto;

DocantoWin::PDFHandler::PDFHandler(const std::filesystem::path& p, std::shared_ptr<Direct2DRender> render) : m_render(render) {
	m_pdfimageprocessor = std::make_shared<PDFHandlerImageProcessor>(render);
	m_pdf = std::make_shared<PDF>(p);
	m_pdfrender = std::make_shared<PDFRenderer>(m_pdf, m_pdfimageprocessor);
}

void DocantoWin::PDFHandler::draw() {
	auto& preview_info = m_pdfrender->get_preview();

	for (const auto& info : preview_info) {
		//m_render->draw_bitmap(info.pos, m_pdfimageprocessor->m_all_bitmaps[info.id]);
	}

	auto& highdef = m_pdfrender->draw();

	for (const auto& info : highdef) {
		m_render->draw_bitmap(info.recs, m_pdfimageprocessor->m_all_bitmaps[info.id]);
	}

	m_pdfrender->debug_draw(m_render);
}

std::shared_ptr<Docanto::PDF> DocantoWin::PDFHandler::get_pdf() const {
	return m_pdf;
}

void DocantoWin::PDFHandler::render() {
	m_pdfrender->render();

}

void DocantoWin::PDFHandler::request() {
	auto scale = WINDOWS_DEFAULT_DPI / m_render->get_dpi();
	auto dims = m_render->get_attached_window()->get_client_size();
	auto local_rec =  m_render->inv_transform({ 0, 0, (float)dims.width * scale, (float)dims.height * scale});


	m_pdfrender->request(local_rec, m_render->get_transform_scale() * m_render->get_dpi());
}

DocantoWin::PDFHandler::PDFHandlerImageProcessor::PDFHandlerImageProcessor(std::shared_ptr<Direct2DRender> render) {
	m_render = render;
}

void DocantoWin::PDFHandler::PDFHandlerImageProcessor::processImage(size_t id, const Docanto::Image& img) {
	m_all_bitmaps[id] = m_render->create_bitmap(img);
}

void DocantoWin::PDFHandler::PDFHandlerImageProcessor::deleteImage(size_t id) {
	if (m_all_bitmaps.find(id) != m_all_bitmaps.end()) {
		m_all_bitmaps.erase(id);
	}
}
