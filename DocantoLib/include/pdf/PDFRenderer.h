#ifndef _PDFRENDERER_H_
#define _PDFRENDERER_H_

#include "../general/Common.h"
#include "../general/Image.h"
#include "../general/ReadWriteMutex.h"
#include "../general/BasicRender.h"
#include "PDF.h"


namespace Docanto {
	class IPDFRenderImageProcessor {
	public:
		virtual ~IPDFRenderImageProcessor() = default;

		virtual void processImage(size_t id, const Image& img) = 0;
		virtual void deleteImage(size_t id) = 0;
	};

	class PDFRenderer {
		struct PDFRenderInfo {
			size_t id = 0;
			Geometry::Rectangle<float> recs;
			float dpi = 0;
		};

		std::shared_ptr<PDF> pdf_obj;

		// The id that identifies this class (and the pdf)
		size_t id = 0;
		static size_t last_id;

		std::shared_ptr<IPDFRenderImageProcessor> m_processor;

		// this function gets called when a bitmaps was processed
		std::function<void(size_t)> m_render_callback;

		float m_standard_dpi = 96;
		float m_preview_dpi = MUPDF_DEFAULT_DPI;
		float m_margin = 1;

		void remove_from_processor(size_t id);
		void add_to_processor();

		size_t remove_finished_queue_item();
		size_t abort_queue_item();
		size_t cull_bitmaps();
		size_t cull_chunks(std::vector<Geometry::Rectangle<float>>& chunks, size_t pagee);

		void process_chunks(const std::vector<Geometry::Rectangle<float>>& chunks, size_t page);
		void create_preview(float dpi = MUPDF_DEFAULT_DPI);
		void position_pdfs();

		std::pair<std::vector<Geometry::Rectangle<float>>, float> get_chunks(size_t page);
		std::pair<float, float> get_chunk_dpi_bound();

		void async_render(); 
		void receive_image(PDFRenderInfo info, Image&& i);
	public:
		PDFRenderer(std::shared_ptr<PDF> pdf_obj, std::shared_ptr<IPDFRenderImageProcessor> processor);
		~PDFRenderer();

		Image get_image(size_t page, float dpi = MUPDF_DEFAULT_DPI);

		const std::vector<PDFRenderInfo>& get_preview();
		Docanto::ReadWrapper<std::vector<Docanto::PDFRenderer::PDFRenderInfo>> draw();
		std::vector<Geometry::Rectangle<double>> get_clipped_page_recs();

		void request(Geometry::Rectangle<float> view, float dpi);
		void render();
		void set_rendercallback(std::function<void(size_t)> fun);
				
		void debug_draw(std::shared_ptr<BasicRender> render);

		void update();
	private:
		struct impl;
		class RenderThreadManager;
		static std::unique_ptr<RenderThreadManager> thread_manager;
		static size_t tread_manager_count;

		std::unique_ptr<impl> pimpl;
	};
}


#endif // !_PDFRENDERER_H_
