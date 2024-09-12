#pragma once

#define PDF_STITCH_THRESHOLD 1.0f
#include "mupdf/fitz.h"
#include "mupdf/pdf.h"
#include "MuPDF.h"
#include "../Windows/Direct2D.h"
#include "../Windows/WindowHandler.h"

#ifndef _PDF_HANDLER_H_
#define _PDF_HANDLER_H_
/// <summary>
/// Class to handle the rendering of only one pdf
/// </summary>
class PDFRenderHandler {
public:
	struct RenderInstructions {
		bool draw_content = true;
		bool draw_annots = true;
		bool draw_preview = true;

		bool render_highres = true;
		bool render_annots = true;
		bool render_preview = true;
		bool draw_outline = true;
	};

	struct RenderInfo {
		enum STATUS {
			WAITING,
			IN_PROGRESS,
			FINISHED,
			ABORTED
		};

		enum JOB_TYPE {
			HIGH_RES,
			PREVIEW,
			ANNOTATION,
			RELOAD_ANNOTATIONS,
			UNKNOWN
		};

		size_t page = 0;
		fz_cookie* cookie = nullptr;
		STATUS stat = WAITING;
		Math::Rectangle<float> overlap_in_docspace;

		float dpi = MUPDF_DEFAULT_DPI;
		JOB_TYPE job = UNKNOWN;

		// This if for use in the reload of the annotations
		// so the thread know if they already reloaded the annotations
		std::vector<std::thread::id> thread_ids;

		RenderInfo() = default;
		RenderInfo(size_t page, float dpi, Math::Rectangle<float> overlap) : page(page), dpi(dpi), overlap_in_docspace(overlap) {
			cookie = new fz_cookie({ 0,0,0,0,0 });
		}
		RenderInfo(size_t page, float dpi, Math::Rectangle<float> overlap, JOB_TYPE job) : page(page), dpi(dpi), overlap_in_docspace(overlap), job(job) {
			cookie = new fz_cookie({ 0,0,0,0,0 });
		}
		// move constructor
		RenderInfo(RenderInfo&& t) noexcept {
			page = t.page;
			dpi = t.dpi;
			cookie = t.cookie;
			t.cookie = nullptr;
			stat = t.stat;
			overlap_in_docspace = t.overlap_in_docspace;
			job = t.job;
			thread_ids = std::move(t.thread_ids);
		}
		// move assignment
		RenderInfo& operator=(RenderInfo&& t) noexcept {
			this->~RenderInfo();
			new(this) RenderInfo(std::move(t));
			return *this;
		}

		~RenderInfo() {
			delete cookie;
		}
	};

private:
	struct DisplayListContent {
		std::shared_ptr<DisplayListWrapper> m_page_content, m_page_annots, m_page_widgets;
	};
	/**
	* List by which ThreadSafeItems should be acquired
	* 1. m_pdf member
	* 2. m_pagerec
	* 3. m_preview_bitmaps
	* 4. m_display_list
	* 5. m_highres_bitmaps
	* 6. m_render_queue
	*/

	// not owned by this class
	MuPDFHandler::PDF* const m_pdf = nullptr;
	// not owned by this class!
	Direct2DRenderer* const m_renderer = nullptr;
	// not owned by this class!
	WindowHandler* const m_window = nullptr;

	// --- Display list generation ---
	std::shared_ptr<ThreadSafeVector<DisplayListContent>> m_display_list;
	std::thread m_display_list_thread;

	std::atomic_bool  m_display_list_processed = false;
	std::atomic_size_t m_display_list_amount_processed = 0;
	std::atomic_size_t m_display_list_amount_processed_total = 0;

	// --- For Multithreading ---
	// General Multithreading
	std::atomic_bool m_should_threads_die = false;
	std::atomic_bool m_render_jobs_available = false;
	std::atomic_long m_amount_thread_running = 0;
	std::vector<std::thread> m_render_threads;
	std::shared_ptr<ThreadSafeDeque<std::shared_ptr<RenderInfo>>> m_render_queue = nullptr;

	std::mutex m_render_queue_mutex;
	std::condition_variable m_render_queue_condition_var;
	// High res rendering
	std::shared_ptr<ThreadSafeDeque<CachedBitmap>> m_highres_bitmaps = nullptr;
	// Preview rendering
	std::atomic<float> m_preview_dpi = 0.1f * MUPDF_DEFAULT_DPI;
	std::shared_ptr<ThreadSafeDeque<CachedBitmap>> m_preview_bitmaps = nullptr;
	// Annotations rendering
	std::shared_ptr<ThreadSafeDeque<CachedBitmap>> m_annotation_bitmaps = nullptr;
	std::shared_ptr<ThreadSafeDeque<CachedBitmap>> m_annotation_bitmaps_old = nullptr;

	void create_display_list();

	void create_render_job(RenderInstructions r);

	void send_bitmaps(RenderInstructions r);

	std::vector<PDFRenderHandler::RenderInfo> get_pdf_overlap(RenderInfo::JOB_TYPE type, std::vector<size_t>& cached_bitmap_index, std::optional<float> dpi_check, float dpi_margin = 0.0f, size_t limit = 0);
	std::vector<size_t> get_page_overlap();

	void remove_unused_queue_items();
	void update_render_queue();
	void async_render();

	void remove_small_cached_bitmaps(float treshold);

	/// <param name="max_memory">In Mega Bytes</param>
	void reduce_cache_size(std::shared_ptr<ThreadSafeDeque<CachedBitmap>> target, unsigned long long max_memory);
	unsigned long long get_cache_memory_usage(std::shared_ptr<ThreadSafeDeque<CachedBitmap>> target) const;


public:
	PDFRenderHandler() = default;

	PDFRenderHandler(MuPDFHandler::PDF* const pdf, Direct2DRenderer* const renderer, WindowHandler* const window, size_t amount_threads = 2);

	~PDFRenderHandler();

	/// <summary>
	/// Will render out the pdf page to a bitmap. It will always give back a bitmap with the DPI 96.
	/// To account for higher DPI's it will instead scale the size of the image using <c>get_page_size </c> function
	/// </summary>
	/// <param name="renderer">Reference to the Direct2D renderer</param>
	/// <param name="page">The page to be rendererd</param>
	/// <param name="dpi">The dpi at which it should be rendered</param>
	/// <returns>The bitmap object</returns>
	Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, float dpi = 72.0f) const;
	/// <summary>
	/// Will render out the pdf page to a bitmap with the given size
	/// </summary>
	/// <param name="renderer">Reference to the Direct2D renderer</param>
	/// <param name="page">The page to be rendererd</param>
	/// <param name="rec">The end size of the Bitmap</param>
	/// <returns>Bitmap</returns>
	Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, Math::Rectangle<unsigned int> rec) const;
	/// <summary>
	/// Will render out the area of the pdf page given by the source rectangle to a bitmap with the given dpi. 
	/// </summary>
	/// <param name="renderer">Reference to the Direct2D renderer</param>
	/// <param name="page">The page to be rendererd</param>
	/// <param name="source">The area of the page in docpage pixels. Should be smaller than get_page_size() would give</param>
	/// <param name="dpi">The dpi at which it should be rendered</param>
	/// <returns>Returns the DPI-scaled bitmap</returns>
	/// <remark>If the DPI is 72 the size of the returned bitmap is the same as the size of the source</remark>
	Direct2DRenderer::BitmapObject get_bitmap(Direct2DRenderer& renderer, size_t page, Math::Rectangle<float> source, float dpi) const;

	void render_preview();
	void render_outline();
	void render();
	/// <summary>
	/// Will render out the pdf page with the given instructions.
	/// <para>
	/// Notes: Due to limitations of the implementation, the render_preview will always render 
	/// the whole page as if render_content, render_widget and render_annot is true
	/// </para>
	/// </summary>
	/// <param name="instruct"></param>
	void render(RenderInstructions instruct);
	/// <summary>
	/// This function should be called if the following render instructions change
	/// <para>
	/// 1. render_content
	/// </para>
	/// <para>
	/// 2. render_widgets
	/// </para>
	/// <para>
	/// 3. render_annots 
	/// </para>
	/// It will force to rerender the whole pdf
	/// </summary>
	void clear_render_cache();
	void clear_annotation_cache();
	void update_annotations(size_t page);
	float get_display_list_progress();

};

#endif // !_PDF_HANDLER_H_