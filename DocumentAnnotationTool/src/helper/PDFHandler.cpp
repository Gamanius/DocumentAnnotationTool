#include "include.h"

void PDFHandler::create_preview(float scale) {
	for (size_t i = 0; i < m_pdf.get_page_count(); i++) {
		m_previewBitmaps.push_back(m_pdf.get_bitmap(*m_renderer, i, MUPDF_DEFAULT_DPI * scale));
	}

	m_preview_scale = scale;
}

PDFHandler::PDFHandler(Direct2DRenderer* const renderer, MuPDFHandler& handler, const std::wstring& path) : m_renderer(renderer) {
	// will raise error if path is invalid so we dont need to do assert here
	auto d = handler.load_pdf(path);
	if (!d.has_value()) {
		return;
	}
	m_pdf = std::move(d.value());

	create_preview(); // create preview with default scale
}

PDFHandler::PDFHandler(PDFHandler&& t) noexcept : m_renderer(t.m_renderer) {
	m_pdf = std::move(t.m_pdf); 
	m_previewBitmaps = std::move(t.m_previewBitmaps); 

	m_preview_scale = t.m_preview_scale;
	t.m_preview_scale = 0;

	m_seperation_distance = t.m_seperation_distance;
	t.m_seperation_distance = 0;
}

PDFHandler& PDFHandler::operator=(PDFHandler&& t) noexcept {
	// new c++ stuff?
	this->~PDFHandler();
	new(this) PDFHandler(std::move(t)); 
	return *this;
}

void PDFHandler::render_preview(Renderer::Rectangle<double> screen_clip_coords) {
	// need to add up all the pdf pages because they could be different heights
	double height = 0;

	for (size_t i = 0; (i < m_pdf.get_page_count()) and height < screen_clip_coords.y; i++) {
		height += m_pdf.get_page_size(i).height; 
		height += m_seperation_distance; 
	}
	// TODO

}

void PDFHandler::render(Renderer::Rectangle<double> screen_clip_coords) {
	auto scale = m_renderer->get_transform_scale();

	if (scale > m_preview_scale) {
		// if we are zoomed in, we need to render the page at a higher resolution
		// than the screen resolution
	}
	else {
		render_preview(screen_clip_coords);
	}
}

Renderer::Rectangle<double> PDFHandler::get_bounding_box_of_page_in_screen_coords(size_t page) const {
	// need to add up all the pdf pages because they could be different heights
	double height = 0;
	for (size_t i = 0; i < page; i++) {
		height += m_pdf.get_page_size(i).height;
		height += m_seperation_distance;
	}
	auto size = m_pdf.get_page_size(page);
	return Renderer::Rectangle<double>(0, height, size.width, size.height);
}

PDFHandler::~PDFHandler() {
	//m_pdf will be destroyed automatically
	//m_preview will be destroyed automatically
}
