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
		m_render->draw_bitmap(info.pos, m_pdfimageprocessor->m_all_bitmaps[info.id]);
	}
}

std::shared_ptr<Docanto::PDF> DocantoWin::PDFHandler::get_pdf() const {
	return m_pdf;
}

void DocantoWin::PDFHandler::render() {

}

DocantoWin::PDFHandler::PDFHandlerImageProcessor::PDFHandlerImageProcessor(std::shared_ptr<Direct2DRender> render) {
	m_render = render;
}

void DocantoWin::PDFHandler::PDFHandlerImageProcessor::processImage(size_t id, const Docanto::Image& img) {
	m_all_bitmaps[id] = m_render->create_bitmap(img);
}
