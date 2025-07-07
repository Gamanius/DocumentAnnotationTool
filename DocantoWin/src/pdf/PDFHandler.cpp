#include "PDFHandler.h"

using namespace Docanto;

DocantoWin::PDFHandler::PDFHandler(const std::filesystem::path& p, std::shared_ptr<Direct2DRender> render) : m_render(render) {
	m_pdf = std::make_shared<PDF>(p);
	m_pdfrender = std::make_shared<PDFRenderer>(m_pdf);
}

void DocantoWin::PDFHandler::draw() {
	
	for (size_t i = 0; i < m_bitmaps.size(); i++) {
		m_render->draw_bitmap({ 0, 0 }, *(m_bitmaps[i].get()));
	}
}

std::shared_ptr<Docanto::PDF> DocantoWin::PDFHandler::get_pdf() const {
	return m_pdf;
}

void DocantoWin::PDFHandler::render() {
	auto i = m_pdfrender->get_image(0);
	m_bitmaps.emplace_back(std::make_unique<Direct2DRender::BitmapObject>(m_render->create_bitmap(i)));

}