#ifndef _DOCANTOWIN_PDFHANDLER_H_
#define _DOCANTOWIN_PDFHANDLER_H_

#include "../include.h"

#include "win/Direct2D.h"

namespace DocantoWin {
	class PDFHandler {
	public:
		struct PDFWrapper {
			std::shared_ptr<Docanto::PDF> pdf;
			std::shared_ptr<Docanto::PDFRenderer> render;
			std::shared_ptr<Docanto::PDFAnnotation> annotation;
		};
	private:
		std::vector<PDFWrapper> m_pdfobj;

		struct PDFHandlerImageProcessor : public Docanto::IPDFRenderImageProcessor {
			Docanto::ThreadSafeWrapper<std::map<size_t, Direct2DRender::BitmapObject>> m_all_bitmaps;
			std::shared_ptr<Direct2DRender> m_render;

			PDFHandlerImageProcessor(std::shared_ptr<Direct2DRender> render);

			void processImage(size_t id, const Docanto::Image& img) override;
			void deleteImage(size_t id) override;
		};

		std::shared_ptr<PDFHandlerImageProcessor> m_pdfimageprocessor;
		std::shared_ptr<Direct2DRender> m_render;

		bool m_debug_draw = false;

		void draw(std::shared_ptr<Docanto::PDFRenderer> r);
	public:
		PDFHandler(const std::filesystem::path& p, std::shared_ptr<Direct2DRender> render);

		void add_pdf(const std::filesystem::path& p);

		void request();
		void draw();
		void set_debug_draw(bool b = true);
		void toggle_debug_draw();

		void reload();

		std::pair<PDFWrapper, size_t> get_pdf_at_point(Docanto::Geometry::Point<float> p);

		size_t get_amount_of_pdfs() const;
	};
}

#endif // !_DOCANTOWIN_PDFHANDLER_H_
