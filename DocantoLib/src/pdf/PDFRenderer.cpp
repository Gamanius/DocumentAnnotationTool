#include "PDFRenderer.h"

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

#include "../../include/general/Timer.h"
#include "../../include/general/ReadWriteMutex.h"

#include <unordered_set>

#define FLOAT_EQUAL(a, b) (std::abs(a - b) < 0.001)

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

struct DisplayListWrapper : public Docanto::ThreadSafeWrapper<fz_display_list*> {
	using Docanto::ThreadSafeWrapper<fz_display_list*>::ThreadSafeWrapper;

	~DisplayListWrapper() {
		auto d = get().get();

		if (d != nullptr) {
			auto ctx = Docanto::GlobalPDFContext::get_instance().get();
			fz_drop_display_list(*ctx, *d);
		}
	}
};

Docanto::Image get_image_from_list(fz_context* ctx, fz_display_list* wrap, const Docanto::Geometry::Rectangle<float>& scissor, const float dpi, fz_cookie* cookie = nullptr);

class Docanto::PDFRenderer::RenderThreadManager {
	bool m_should_worker_die = false;
public:
	enum class RenderStatus {
		WAITING,
		PROCESSING,
		DONE,
		CANCELD
	};

	enum class JobType {
		RENDER_BITMAP,
		LOAD_DISPLAY_LIST,
		DELETE_DISPLAY_LIST
	};

	enum class ContentType {
		CONTENT,
		ANNOTATION,
		WIDGET
	};

	struct RenderJob {
		JobType job = JobType::RENDER_BITMAP;
		size_t callback_id = 0;

		// For rendering jobs these information are needed
		PDFRenderInfo info;
		Geometry::Rectangle<float> chunk_rec;
		RenderStatus status = RenderStatus::WAITING;
		ContentType type = ContentType::CONTENT;
		fz_cookie cookie = {};

		// The list to copy
		std::shared_ptr<DisplayListWrapper> list = nullptr;
		std::vector<std::thread::id> threads_already_copied;

		// for debug only
		std::thread::id render_id;
	};

	std::atomic_size_t m_last_id = 0;

private:
	std::vector<std::thread> m_render_worker;

	// the queue needs to be synchronized using the below mutex!
	std::deque<std::shared_ptr<RenderJob>> m_jobs;
	std::mutex m_render_worker_queue_mutex;
	std::condition_variable m_render_worker_queue_condition_var;

	std::map<size_t, std::function<void(PDFRenderInfo, Image&&)>> m_job_callback;

	struct pair_hash {
		inline std::size_t operator()(const std::tuple<size_t, size_t, ContentType>& v) const {
			switch (std::get<2>(v)) {
				case ContentType::CONTENT:		return std::get<0>(v)* 31 + std::get<1>(v);
				case ContentType::ANNOTATION:	return std::get<0>(v)* 31 + std::get<1>(v) + 1;
				case ContentType::WIDGET:		return std::get<0>(v)* 31 + std::get<1>(v) + 2;
			}
			return std::get<0>(v) * 31 + std::get<1>(v) + 7;
		}
	};
	std::unordered_set<std::tuple<size_t, size_t, ContentType>, pair_hash> m_display_list_cache;

public:
	void add_job(size_t id, std::shared_ptr<RenderJob> job) {
		std::scoped_lock<std::mutex> lock(m_render_worker_queue_mutex);
		job->callback_id = id;

		if (job->job == JobType::LOAD_DISPLAY_LIST ) {
			m_display_list_cache.insert({ id, job->info.page, job->type });
			m_jobs.push_back(job);
		}
		else if (job->job == JobType::DELETE_DISPLAY_LIST) {
			if (m_display_list_cache.contains({ id, job->info.page, job->type })) {
				m_display_list_cache.erase({ id, job->info.page, job->type });
			}
			else {
				return;
			}
			m_jobs.push_front(job);
		}
		else {
			m_jobs.push_back(job);
		}
		
		m_render_worker_queue_condition_var.notify_one();
	}

	void set_callback(size_t id, std::function<void(PDFRenderInfo, Image&&)> f) {
		m_job_callback[id] = f;
	}
	
	bool has_display_list(size_t id, size_t page, ContentType type) {
		return m_display_list_cache.contains({ id, page, type });
	}

	size_t get_amount_threads() {
		return m_render_worker.size();
	}

	size_t remove_finished_jobs() {
		std::scoped_lock<std::mutex> lock(m_render_worker_queue_mutex);
		auto& queue = m_jobs;
		size_t amount = 0;

		for (auto it = queue.begin(); it != queue.end();) {
			if ((*it)->status == RenderStatus::DONE or (*it)->threads_already_copied.size() == get_amount_threads()) {
				(*it)->status = RenderStatus::DONE;
				queue.erase(it);
				amount++;
				it = queue.begin();
			}
			else {
				it++;
			}
		}

		return amount;
	}

	void async_render() {
		Timer start;
		fz_context* ctx;
		{
			auto c = GlobalPDFContext::get_instance().get();
			ctx = fz_clone_context(*c);
		}

		auto copy_list = [ctx](fz_display_list* list) {
			fz_display_list* ls = fz_new_display_list(ctx, (list)->mediabox);
			ls->len = (list)->len;
			ls->max = (list)->max;
			ls->list = new fz_display_node[ls->max];
			memcpy(ls->list, (list)->list, (list)->len * sizeof(fz_display_node));
			return ls;
			};

		auto delete_list = [ctx](fz_display_list* list) {
			delete[] list->list;
			list->list = nullptr;
			list->len = 0;
			list->max = 0;
			fz_drop_display_list(ctx, list);
			};

		std::map<std::tuple<size_t, size_t, ContentType>, fz_display_list*> t_content_list;
		
		Docanto::Logger::log("Initialized render thread in ", start);
		auto local_thread_id = std::this_thread::get_id();

		while (!m_should_worker_die) {
			// wait until a signal is received
			std::unique_lock<std::mutex> lock(m_render_worker_queue_mutex);
			m_render_worker_queue_condition_var.wait(lock, [this] {
				return m_should_worker_die or m_jobs.size() != 0;
			});

			// get a job
			std::shared_ptr<RenderJob> current_job = nullptr;
			auto& queue = m_jobs;
			for (size_t i = 0; i < queue.size(); i++) {
				auto& job = queue.at(i);
				
				if (job->job == JobType::LOAD_DISPLAY_LIST) {
					// check if the job was once completed by this thread
					if (std::find(job->threads_already_copied.begin(), job->threads_already_copied.end(), local_thread_id) != job->threads_already_copied.end()
						// or check if the display list already exist
						or t_content_list.contains({ job->callback_id, job->info.page, job->type })) {
						continue;
					}
					//Docanto::Logger::log("Loading list page ", job->info.page);
					current_job = job;
					break;
				}

				if (job->job == JobType::DELETE_DISPLAY_LIST) {
					// check if the thread already handled it
					if (std::find(job->threads_already_copied.begin(), job->threads_already_copied.end(), local_thread_id) != job->threads_already_copied.end() 
						// or the list was already deleted at sometime. we cannot delete the same list twice
						or !t_content_list.contains({ job->callback_id, job->info.page, job->type })) {
						continue;
					}
					current_job = job;
					//Docanto::Logger::log("Deleting list page ", job->info.page);
					break;
				}

				// accept any jobs that are currently in the queue
				if (job->job == JobType::RENDER_BITMAP and job->status == RenderStatus::WAITING) {
					// first check if the job was aborted
					if (job->cookie.abort == 1) {
						job->status = RenderStatus::DONE;
						continue;
					}

					// second check if the thread does have the methods to render the bitmap
					if (!t_content_list.contains({ job->callback_id, job->info.page, job->type })) {
						// if it doesnt we continue
						continue;
					}

					job->status = RenderStatus::PROCESSING;
					job->render_id = std::this_thread::get_id();
					// we need to safe the pointer since we cannot retrieve it later without being const
					current_job = job;
					//Docanto::Logger::log("Accepting job ", job->info.id);
					break;
				}
			}

			// unlock
			lock.unlock();

			// we didnt find any rendering jobs and we can go back to waiting
			if (current_job == nullptr) {
				remove_finished_jobs();
				continue;
			}

			if (current_job->job == JobType::RENDER_BITMAP) {
				auto list = t_content_list[{current_job->callback_id, current_job->info.page, current_job->type}];
				auto cont_img = get_image_from_list(ctx, list, current_job->chunk_rec, current_job->info.dpi, &(current_job->cookie));
						
				// we have to check if the rendering was aborted
				if (current_job->cookie.abort) {
					current_job->status = RenderStatus::DONE;
					continue;
				}
				

				if (m_job_callback.contains(current_job->callback_id)) {
					m_job_callback[current_job->callback_id](current_job->info, std::move(cont_img));
				}
				
				current_job->status = RenderStatus::DONE;
				continue;
			}

			if (current_job->job == JobType::LOAD_DISPLAY_LIST) {
				t_content_list[{current_job->callback_id, current_job->info.page, current_job->type}] = copy_list(*(current_job->list->get().get()));
				lock.lock();
				current_job->threads_already_copied.push_back(local_thread_id);;
				lock.unlock();
				continue;
			}

			if (current_job->job == JobType::DELETE_DISPLAY_LIST) {
				if (!t_content_list.contains({ current_job->callback_id, current_job->info.page, current_job->type }))
					continue;

				delete_list(t_content_list[{current_job->callback_id, current_job->info.page, current_job->type}]);
				t_content_list.erase({ current_job->callback_id, current_job->info.page, current_job->type });

				lock.lock();
				current_job->threads_already_copied.push_back(local_thread_id);
				lock.unlock();
			}
			
		}

		for (auto& [_,list] : t_content_list) {
			delete_list(list);
		}

		fz_drop_context(ctx);
	}

	RenderThreadManager() {
		// add two threads for now
		m_render_worker.push_back(std::thread([this] { async_render(); }));
		m_render_worker.push_back(std::thread([this] { async_render(); }));
	}

	~RenderThreadManager() {
		m_should_worker_die = true;
		m_render_worker_queue_condition_var.notify_all();

		for (auto& thread : m_render_worker) {
			thread.join();
		}
	}
};


struct Docanto::PDFRenderer::impl {
	ThreadSafeVector<std::shared_ptr<DisplayListWrapper>> m_page_content;
	ThreadSafeVector<std::shared_ptr<DisplayListWrapper>> m_page_widgets;
	ThreadSafeVector<std::shared_ptr<DisplayListWrapper>> m_page_annotat;
	std::vector<Geometry::Point<float>> m_page_pos;

	ThreadSafeVector<PDFRenderInfo> m_previewbitmaps;
	ThreadSafeVector<PDFRenderInfo> m_highDefBitmaps;
	ThreadSafeVector<PDFRenderInfo> m_annotationBitmaps;

	// the queue needs to be synchronized using the below mutex!
	ThreadSafeWrapper<std::deque<std::shared_ptr<RenderThreadManager::RenderJob>>> m_jobs;

	Geometry::Rectangle<float> m_current_viewport;
	float m_current_dpi = 92;

	impl() = default;
	~impl() = default;
};



std::unique_ptr<Docanto::PDFRenderer::RenderThreadManager> Docanto::PDFRenderer::thread_manager = nullptr;

size_t Docanto::PDFRenderer::tread_manager_count = 0;
size_t Docanto::PDFRenderer::last_id = 1;

Docanto::PDFRenderer::PDFRenderer(std::shared_ptr<PDF> pdf_obj, std::shared_ptr<IPDFRenderImageProcessor> processor) : pimpl(std::make_unique<impl>()) {
	if (thread_manager == nullptr) {
		thread_manager = std::make_unique<RenderThreadManager>();
		tread_manager_count++;
	}

	id = last_id++;

	this->pdf_obj = pdf_obj;
	this->m_processor = processor;

	thread_manager->set_callback(id, [&](PDFRenderInfo info, Image&& i) {receive_image(info, std::move(i)); });

	position_pdfs();
	//create_preview();
	update();

}

Docanto::PDFRenderer::~PDFRenderer() {
	tread_manager_count--;

	if (tread_manager_count == 0) {
		thread_manager.reset();
	}
}

Docanto::Image get_image_from_list(fz_context* ctx, fz_display_list* wrap, const Docanto::Geometry::Rectangle<float>& scissor, const float dpi, fz_cookie* cookie) {
	// now we can render it
	auto fz_scissor = fz_make_rect(scissor.x, scissor.y, scissor.right(), scissor.bottom());
	auto ctm = fz_transform_page(fz_scissor, dpi, 0);

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;
	
	auto bound = fz_transform_rect(fz_scissor, ctm);
	auto bbox = fz_round_rect(bound);

	Docanto::Image obj;
	fz_try(ctx) {
		// ___---___ Rendering part ___---___
		// create new pixmap
		pixmap = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 1);
		// create draw device
		drawdevice = fz_new_draw_device(ctx, fz_identity, pixmap);
		// render to draw device
		fz_clear_pixmap(ctx, pixmap); // for transparent background
		fz_run_display_list(ctx, wrap, drawdevice, ctm, bound, cookie);

		fz_close_device(ctx, drawdevice);
		// ___---___ Bitmap creatin and display part ___---___
		// create the bitmap
		obj.data = std::unique_ptr<byte>(pixmap->samples); // , size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
		obj.dims = { (size_t)(bbox.x1 - bbox.x0), (size_t)(bbox.y1 - bbox.y0) };
		obj.size = pixmap->h * pixmap->stride;
		obj.stride = pixmap->stride;
		obj.components = pixmap->n;
		obj.dpi = dpi;

		pixmap->samples = nullptr;

	} fz_always(ctx) {
		// drop all devices
		fz_drop_device(ctx, drawdevice);
		fz_drop_pixmap(ctx, pixmap);
	} fz_catch(ctx) {
		if (cookie->abort == 1) {
			fz_ignore_error(ctx);
		}
		else {
			Docanto::Logger::error("MUPdf Error: ", fz_convert_error(ctx, &((ctx)->error.errcode)));
		}
	}
	return obj;
}

Docanto::Image get_image_from_list(DisplayListWrapper* wrap, Docanto::Geometry::Rectangle<float> scissor, float dpi) {
	return get_image_from_list(*(Docanto::GlobalPDFContext::get_instance().get()), *(wrap->get().get()), scissor, dpi);
}

float Docanto::PDFRenderer::get_chunk_scale() const {
	return std::floor(pimpl->m_current_dpi / MUPDF_DEFAULT_DPI);
}

float Docanto::PDFRenderer::get_chunk_dpi() const {
	return (get_chunk_scale() + 1) * MUPDF_DEFAULT_DPI;
}

std::pair<std::vector<Docanto::Geometry::Rectangle<float>>, float> Docanto::PDFRenderer::get_chunks(size_t page) {
	auto dims = pdf_obj->get_page_dimension(page);
	auto  pos = pimpl->m_page_pos.at(page);

	float scale = std::max(get_chunk_scale(), 0.0001f); // to avoid negative values
	size_t amount_cells = std::max<size_t>(static_cast<size_t>(std::max<float>(std::log(scale) * 5, 1.0f)), 1);
	auto amount_cells_w = std::max<size_t>(amount_cells * dims.width  / 600.0, 1);
	auto amount_cells_h = std::max<size_t>(amount_cells * dims.height / 850.0, 1);
	Geometry::Dimension<float> cell_dim = { dims.width / amount_cells_w, dims.height / amount_cells_h };
	// transform the viewport to docspace
	auto doc_space_screen = Docanto::Geometry::Rectangle<float>(pimpl->m_current_viewport.upperleft() - pos, pimpl->m_current_viewport.lowerright() - pos);

	Geometry::Point<size_t> topleft = {
		(size_t)std::clamp(std::floor(doc_space_screen.upperleft().x /  dims.width * amount_cells_w), 0.0f, (float)amount_cells_w),
		(size_t)std::clamp(std::floor(doc_space_screen.upperleft().y / dims.height * amount_cells_h), 0.0f, (float)amount_cells_h),
	};

	Geometry::Point<size_t> bottomright = {
		(size_t)std::clamp(std::ceil(doc_space_screen.lowerright().x / dims.width * amount_cells_w), 0.0f, (float)amount_cells_w),
		(size_t)std::clamp(std::ceil(doc_space_screen.lowerright().y / dims.height * amount_cells_h), 0.0f, (float)amount_cells_h),
	};

	auto chunks = std::vector<Docanto::Geometry::Rectangle<float>>();
	auto dx = bottomright.x - topleft.x;
	auto dy = bottomright.y - topleft.y;
	chunks.reserve(dx * dy);

	for (size_t x = topleft.x; x < bottomright.x; ++x) {
		for (size_t y = topleft.y; y < bottomright.y; ++y) {
			chunks.push_back({
				x * cell_dim.width - m_margin,
				y * cell_dim.height - m_margin,
				cell_dim.width + m_margin,
				cell_dim.height + m_margin
				});
		}
	}

	return { chunks, get_chunk_dpi() };
}

Docanto::Image Docanto::PDFRenderer::get_image(size_t page, float dpi) {
	float scale = dpi / MUPDF_DEFAULT_DPI;
	fz_matrix ctm = fz_scale(scale, scale);

	auto ctx = GlobalPDFContext::get_instance().get();
	auto doc = pdf_obj->get();
	auto pag = pdf_obj->get_page(page).get();

	auto page_bounds = fz_bound_page(*ctx, *pag);

	fz_rect transformed = fz_transform_rect(page_bounds, ctm);
	fz_irect device_bbox = fz_round_rect(transformed);
	int w = device_bbox.x1 - device_bbox.x0;
	int h = device_bbox.y1 - device_bbox.y0;

	fz_pixmap* pixmap = nullptr;
	fz_device* drawdevice = nullptr;

	Image obj;

	fz_try(*ctx) {
		pixmap = fz_new_pixmap_with_bbox(*ctx, fz_device_rgb(*ctx), device_bbox, nullptr, 1);
		fz_clear_pixmap_with_value(*ctx, pixmap, 0xff); // for the white background

		drawdevice = fz_new_draw_device(*ctx, fz_identity, pixmap);
		fz_run_page(*ctx, *pag, drawdevice, ctm, nullptr);
		fz_close_device(*ctx, drawdevice);

		obj.data = std::unique_ptr<byte>(pixmap->samples); // , size, (unsigned int)pixmap->stride, 96); // default dpi of the pixmap
		obj.dims = { size_t(w), size_t(h) };
		obj.size = pixmap->h * pixmap->stride;
		obj.stride = pixmap->stride;
		obj.components = pixmap->n;
		obj.dpi = dpi;

		pixmap->samples = nullptr;
	} fz_always(*ctx) {
		fz_drop_device(*ctx, drawdevice);
		fz_drop_pixmap(*ctx, pixmap);
	} fz_catch(*ctx) {
		return Docanto::Image();
	}

	return obj;
}


void Docanto::PDFRenderer::position_pdfs() {
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto& positions = pimpl->m_page_pos;

	float y = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i);
		positions.push_back({0, y});
		y += dims.height + 10;
	}
}


const std::vector<Docanto::PDFRenderer::PDFRenderInfo>& Docanto::PDFRenderer::get_preview() {
	auto d =  pimpl->m_previewbitmaps.get_read();
	return *d.get();
}

std::vector<Docanto::Geometry::Rectangle<double>> Docanto::PDFRenderer::get_clipped_page_recs() {
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto& positions = pimpl->m_page_pos;
	std::vector<Docanto::Geometry::Rectangle<double>> recs;

	float y = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i);
		Docanto::Geometry::Rectangle<double> rec = { positions[i], dims };
		if (rec.intersects(pimpl->m_current_viewport)) {
			recs.push_back(rec);
		}
	}

	return recs;
}

std::vector<Docanto::Geometry::Rectangle<double>> Docanto::PDFRenderer::get_page_recs() {
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto& positions = pimpl->m_page_pos;
	std::vector<Docanto::Geometry::Rectangle<double>> recs;

	float y = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i);
		Docanto::Geometry::Rectangle<double> rec = { positions[i], dims };
		recs.push_back(rec);
	}

	return recs;
}

Docanto::Geometry::Point<float> Docanto::PDFRenderer::get_position(size_t page) {
	return pimpl->m_page_pos.at(page);
}

void Docanto::PDFRenderer::set_position(size_t page, Geometry::Point<float> pos) {
	pimpl->m_page_pos.at(page) = pos;
}

Docanto::Geometry::Dimension<float> Docanto::PDFRenderer::get_max_dimension() {
	Docanto::Geometry::Dimension<float> dim = { 0, 0 };
	for (size_t i = 0; i < pdf_obj->get_page_count(); i++) {
		auto d = pdf_obj->get_page_dimension(i);
		dim.width = std::max(dim.width, d.width);
		dim.height = std::max(dim.height, d.height);
	}
	return dim;
}


Docanto::ReadWrapper<std::vector<Docanto::PDFRenderer::PDFRenderInfo>> Docanto::PDFRenderer::draw() {
	return pimpl->m_highDefBitmaps.get_read();
}

Docanto::ReadWrapper<std::vector<Docanto::PDFRenderer::PDFRenderInfo>> Docanto::PDFRenderer::annot() {
	return pimpl->m_annotationBitmaps.get_read();
}


void Docanto::PDFRenderer::remove_from_processor(size_t id) {
	auto vec = pimpl->m_highDefBitmaps.get_write();
	std::erase_if(*vec, [id](const auto& obj) {
		return obj.id == id;
	});

	vec = pimpl->m_annotationBitmaps.get_write();
	std::erase_if(*vec, [id](const auto& obj) {
		return obj.id == id;
	});

	m_processor->deleteImage(id);
}

void Docanto::PDFRenderer::add_to_processor() {
	// TODO
}

size_t Docanto::PDFRenderer::cull_bitmaps(ThreadSafeVector<PDFRenderInfo>& info) {
	auto vec = info.get_write();
	std::vector<size_t> ids_to_delete;
	size_t amount = 0;
	auto higher_dpi  = get_chunk_dpi();

	for (auto it = vec->rbegin(); it != vec->rend(); it++) {
		auto& item = *it;
		bool intersetcts = (item.recs + get_position(item.page)).intersects(pimpl->m_current_viewport);
		bool del = false;

		if (intersetcts and // intersection test
			(FLOAT_EQUAL(item.dpi, higher_dpi)) /*dpi test*/) {
			continue;
		}

		if (intersetcts) {
			// check if its intersects with other recs which have our target dpi
			for (auto it2 = vec->rbegin(); it2 != vec->rend(); it2++) {
				if (it == it2) {
					continue;
				}

				if (!FLOAT_EQUAL(it2->dpi, higher_dpi)) {
					continue;
				}

				// if its intersecting with any other rec we can remove it
				if (it2->recs.intersects(item.recs)) {
					del = true;
					break;
				}
			}

			if (del == false) {
				continue;
			}
		}

		ids_to_delete.push_back(item.id);
		amount++;
	}

	for (auto ids : ids_to_delete) {
		remove_from_processor(ids);
	}

	return amount;
}

size_t Docanto::PDFRenderer::cull_chunks(std::vector<Geometry::Rectangle<float>>& chunks, size_t page, ThreadSafeVector<PDFRenderInfo>& info) {
	// TODO we also should look into the queue
	auto render_queue = pimpl->m_jobs.get();
	auto bitmaps = info.get_read();

	size_t amount = 0;
	auto position = pimpl->m_page_pos.at(page);

	for (auto it = chunks.begin(); it != chunks.end();) {
		bool del = false;
		auto chunk = *it;

		// loop over all bitmaps to see if there are any which are already the same 
		for (size_t j = 0; j < bitmaps->size(); j++) {
			auto bitma = bitmaps->at(j);

			if (bitma.page != page or bitma.dpi == 0.0f) {
				continue;
			}

			if (FLOAT_EQUAL(chunk.x, bitma.recs.x) and FLOAT_EQUAL(chunk.y, bitma.recs.y) and
				FLOAT_EQUAL(chunk.width, bitma.recs.width) and FLOAT_EQUAL(chunk.width, bitma.recs.width)) {
				del = true;
				goto DONE;
			}
		}

		for (size_t j = 0; j < render_queue->size(); j++) {
			auto queue_item = render_queue->at(j);

			// only consider the queue items which are on the correct page
			if (queue_item->info.page != page) {
				continue;
			}

			if (FLOAT_EQUAL(chunk.x, queue_item->chunk_rec.x) and FLOAT_EQUAL(chunk.y, queue_item->chunk_rec.y) and
				FLOAT_EQUAL(chunk.width, queue_item->chunk_rec.width) and FLOAT_EQUAL(chunk.width, queue_item->chunk_rec.width)) {
				del = true;
				goto DONE;
			}
		}


	DONE:
		if (del) {
			amount++;
			chunks.erase(it);
			it = chunks.begin();
		}
		else {
			it++;
		}
	}
	
	return amount;
}

void Docanto::PDFRenderer::abort_all_items() {
	auto queue = pimpl->m_jobs.get();
	for (auto& q : *queue) {
		q->cookie.abort = 1;
	}

}

size_t Docanto::PDFRenderer::abort_queue_item() {
	auto queue = pimpl->m_jobs.get();
	size_t amount = 0;

	auto dpi = get_chunk_dpi();

	for (auto it = queue->begin(); it != queue->end(); it++) {
		auto& item = *it;

		if ((item->info.recs + get_position(item->info.page)).intersects(pimpl->m_current_viewport) and // intersection test
			(FLOAT_EQUAL(item->info.dpi, dpi)) /*dpi test*/) {
			continue;
		}

		amount++;

		item->cookie.abort = 1;
	}

	return amount;
}

size_t Docanto::PDFRenderer::remove_finished_queue_item() {
	auto queue = pimpl->m_jobs.get();
	std::erase_if(*queue, [](std::shared_ptr<RenderThreadManager::RenderJob> info) -> bool {
		return info->cookie.abort == 1;
	});

	return 0;
}

void Docanto::PDFRenderer::request(Geometry::Rectangle<float> view, float dpi) {
	auto q_lock = pimpl->m_jobs.get();

	pimpl->m_current_viewport = view;
	pimpl->m_current_dpi = dpi;

	remove_finished_queue_item();
	abort_queue_item();

	cull_bitmaps(pimpl->m_highDefBitmaps);
	cull_bitmaps(pimpl->m_annotationBitmaps);

	size_t amount_of_pages = pdf_obj->get_page_count();
	auto&        positions = pimpl->m_page_pos;


	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i);

		auto pdf_rec = Geometry::Rectangle<float>(positions.at(i), dims);

		// skip pdfs which are currently not visible
		if (!pdf_rec.intersects(pimpl->m_current_viewport)) {
			continue;
		}


		// check for any missing display lists
		if (!thread_manager->has_display_list(id, i, RenderThreadManager::ContentType::CONTENT)) {
			auto job = std::make_shared<RenderThreadManager::RenderJob>();
			job->job = RenderThreadManager::JobType::LOAD_DISPLAY_LIST;
			job->info.page = i;

			// load page content
			job->list = pimpl->m_page_content.get_read()->at(i);
			job->type = RenderThreadManager::ContentType::CONTENT;

			thread_manager->add_job(id, job);
		}

		if (!thread_manager->has_display_list(id, i, RenderThreadManager::ContentType::ANNOTATION)) {
			auto job = std::make_shared<RenderThreadManager::RenderJob>();
			job->job = RenderThreadManager::JobType::LOAD_DISPLAY_LIST;
			job->info.page = i;

			// load annotation content
			job->list = pimpl->m_page_annotat.get_read()->at(i);
			job->type = RenderThreadManager::ContentType::ANNOTATION;

			thread_manager->add_job(id, job);
		}

		auto [content_chunks, dpi] = get_chunks(i);
		auto [anntoation_chunks, _] = get_chunks(i);
		cull_chunks(content_chunks, i, pimpl->m_highDefBitmaps);
		cull_chunks(anntoation_chunks, i, pimpl->m_annotationBitmaps);

		auto queue = pimpl->m_jobs.get();
		auto add_job = [&](RenderThreadManager::ContentType type, Geometry::Rectangle<float> r) -> void {
			auto job = std::make_shared<RenderThreadManager::RenderJob>();
			job->chunk_rec = r;
			job->info.page = i;
			job->status = RenderThreadManager::RenderStatus::WAITING;
			job->type = type;

			job->info.dpi = dpi;
			job->info.id = thread_manager->m_last_id.fetch_add(1);
			job->info.recs = { r.upperleft(), r.dims()};

			job->job = RenderThreadManager::JobType::RENDER_BITMAP;
			queue->push_front(job);
			thread_manager->add_job(id, job);
		};

		for (auto& chunk_rec : content_chunks) {
			// add the new chunks to the queue
			add_job(RenderThreadManager::ContentType::CONTENT, chunk_rec);
		}

		for (auto& chunk_rec : anntoation_chunks) {
			// add the new chunks to the queue
			add_job(RenderThreadManager::ContentType::ANNOTATION, chunk_rec);
		}
	}
}

void Docanto::PDFRenderer::set_rendercallback(std::function<void(size_t)> fun) { m_render_callback = fun; }


void Docanto::PDFRenderer::receive_image(PDFRenderInfo info, Image&& i) {
	// add the new image to the list
	m_processor->processImage(info.id, i);

	// now check what type it is
	auto q = pimpl->m_jobs.get();
	auto iter = std::find_if(q->begin(), q->end(), [&info](std::shared_ptr<RenderThreadManager::RenderJob> item) -> bool {
		return item->info.id == info.id;
	});

	if (iter == q->end()) {
		Logger::error("In receive image: Received an image which was not found in the queue (but how did we send it?)");
		pimpl->m_highDefBitmaps.get_write()->push_back(info);
		return;
	}

	if ((*iter)->type == RenderThreadManager::ContentType::ANNOTATION) {
		pimpl->m_annotationBitmaps.get_write()->push_back(info);
	}
	else {
		pimpl->m_highDefBitmaps.get_write()->push_back(info);
	}

	// we can then remove it from the queue
	q->erase(iter);

	// call the callback
	if (m_render_callback)
		m_render_callback(info.id);
}

void Docanto::PDFRenderer::debug_draw(std::shared_ptr<BasicRender> render) {
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto   positions = pimpl->m_page_pos;

	size_t amount = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i);

		auto pdf_rec = Geometry::Rectangle<float>(positions.at(i), dims);

		if (pdf_rec.intersects(pimpl->m_current_viewport)) {
			amount++;
			auto [recs, _] = get_chunks(i);

			for (auto& r : recs) {
				r = { r.upperleft() + positions.at(i), Geometry::Dimension<float>(r.dims())};
				render->draw_rect(r, { 0, 255 });
			}
			render->draw_rect(pdf_rec, { 255 });
		}
	}


	auto list = draw();
	auto& highdef = *(list);

	for (const auto& info : highdef) {
		render->draw_rect(info.recs + this->get_position(info.page), {0, 0, 255});
	}

}

void Docanto::PDFRenderer::update() {
	// clear the list and copy all again
	Logger::log(L"Start creating Display List");
	Docanto::Timer time;
	// for each rec create a display list
	fz_display_list* list_annot = nullptr;
	fz_display_list* list_widget = nullptr;
	fz_display_list* list_content = nullptr;
	fz_device* dev_annot = nullptr;
	fz_device* dev_widget = nullptr;
	fz_device* dev_content = nullptr;


	auto ctx = GlobalPDFContext::get_instance().get();

	size_t amount_of_pages = pdf_obj->get_page_count();
	//m_display_list_amount_processed_total = amount_of_pages;

	for (size_t i = 0; i < amount_of_pages; i++) {
		// get the page that will be rendered
		auto doc = pdf_obj->get();
		// we have to do a fz call since we want to use the ctx.
		// we can't use any other calls (like m_pdf->get_page()) since we would need to create a ctx
		// wrapper which would be overkill for this scenario.
		fz_page* p = fz_load_page(*ctx, *doc, static_cast<int>(i));
		pdf_update_page(*ctx, reinterpret_cast<pdf_page*>(p));
		fz_try(*ctx) {
			// create a display list with all the draw calls and so on
			list_annot = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));
			list_widget = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));
			list_content = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));

			dev_annot = fz_new_list_device(*ctx, list_annot);
			dev_widget = fz_new_list_device(*ctx, list_widget);
			dev_content = fz_new_list_device(*ctx, list_content);

			// run all three devices
			Timer time2;
			fz_run_page_annots(*ctx, p, dev_annot, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Annots Rendered in ", time2);

			time2 = Timer();
			fz_run_page_widgets(*ctx, p, dev_widget, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Widgets Rendered in ", time2);

			time2 = Timer();
			fz_run_page_contents(*ctx, p, dev_content, fz_identity, nullptr);
			Logger::log("Page ", i + 1, " Content Rendered in ", time2);

			// get the list and add the lists
			auto content = pimpl->m_page_content.get_write();
			content->emplace_back(std::make_unique<DisplayListWrapper>(std::move(list_content)));

			auto widgets = pimpl->m_page_widgets.get_write();
			widgets->emplace_back(std::make_unique<DisplayListWrapper>(std::move(list_widget)));

			auto annotat = pimpl->m_page_annotat.get_write();
			annotat->emplace_back(std::make_unique<DisplayListWrapper>(std::move(list_annot)));


		} fz_always(*ctx) {
			// flush the device
			fz_close_device(*ctx, dev_annot);
			fz_close_device(*ctx, dev_widget);
			fz_close_device(*ctx, dev_content);
			fz_drop_device(*ctx, dev_annot);
			fz_drop_device(*ctx, dev_widget);
			fz_drop_device(*ctx, dev_content);
		} fz_catch(*ctx) {
			Docanto::Logger::error("Could not preprocess the PDF page");
		}
		// always drop page at the end
		fz_drop_page(*ctx, p);
	}

	Logger::log(L"Finished Displaylist in ", time);
}

void Docanto::PDFRenderer::update_page_annotations(size_t page) {
	// clear the list and copy all again
	Docanto::Timer time;
	// for each rec create a display list
	fz_display_list* list_annot = nullptr;
	fz_device* dev_annot = nullptr;

	auto ctx = GlobalPDFContext::get_instance().get();

	// get the page that will be rendered
	auto doc = pdf_obj->get();
	// we have to do a fz call since we want to use the ctx.
	// we can't use any other calls (like m_pdf->get_page()) since we would need to create a ctx
	// wrapper which would be overkill for this scenario.
	fz_page* p = fz_load_page(*ctx, *doc, static_cast<int>(page));
	pdf_update_page(*ctx, reinterpret_cast<pdf_page*>(p));
	fz_try(*ctx) {
		// create a display list with all the draw calls and so on
		list_annot = fz_new_display_list(*ctx, fz_bound_page(*ctx, p));
		dev_annot = fz_new_list_device(*ctx, list_annot);

		// run all three devices
		Timer time2;
		fz_run_page_annots(*ctx, p, dev_annot, fz_identity, nullptr);
		//Logger::log("Page ", page + 1, " Annots Rendered in ", time2);

		// get the list and add the lists
		auto annotat = pimpl->m_page_annotat.get_write();
		annotat->at(page) = std::make_unique<DisplayListWrapper>(std::move(list_annot));
	} fz_always(*ctx) {
		// flush the device
		fz_close_device(*ctx, dev_annot);
		fz_drop_device(*ctx, dev_annot);
	} fz_catch(*ctx) {
		Docanto::Logger::error("Could not process the PDF page");
	}
	// always drop page at the end
	fz_drop_page(*ctx, p);

}

void Docanto::PDFRenderer::reload() {
	// first remove all lists
	size_t amount_of_pages = pdf_obj->get_page_count();

	for (size_t i = 0; i < amount_of_pages; i++) {
		auto job1 = std::make_shared<RenderThreadManager::RenderJob>();
		job1->job = RenderThreadManager::JobType::DELETE_DISPLAY_LIST;
		job1->info.page = i;

		job1->type = RenderThreadManager::ContentType::ANNOTATION;

		auto job2 = std::make_shared<RenderThreadManager::RenderJob>();
		job2->job = RenderThreadManager::JobType::DELETE_DISPLAY_LIST;
		job2->info.page = i;

		job2->type = RenderThreadManager::ContentType::CONTENT;

		thread_manager->add_job(id, job1);
		thread_manager->add_job(id, job2);
	}

	// remove the display lists
	pimpl->m_page_annotat.get_write()->clear();
	pimpl->m_page_content.get_write()->clear();
	pimpl->m_page_widgets.get_write()->clear();


	// remove any bitmaps
	auto content_bitmaps = pimpl->m_highDefBitmaps.get_write();
	for (size_t i = 0; i < content_bitmaps->size(); i++) {
		auto& d = content_bitmaps->at(i);
		remove_from_processor(d.id);
	}

	auto annota_bitmaps = pimpl->m_annotationBitmaps.get_write();
	for (size_t i = 0; i < annota_bitmaps->size(); i++) {
		auto& d = annota_bitmaps->at(i);
		remove_from_processor(d.id);
	}

	content_bitmaps->clear();
	annota_bitmaps->clear();

	// redo them
	update();
}

void Docanto::PDFRenderer::reload_annotations_page(size_t page) {
	//Docanto::Logger::log("-- Reloading Annotations --");
	//abort_all_items();
	auto job2 = std::make_shared<RenderThreadManager::RenderJob>();
	job2->job = RenderThreadManager::JobType::DELETE_DISPLAY_LIST;
	job2->info.page = page;

	job2->type = RenderThreadManager::ContentType::ANNOTATION;

	thread_manager->add_job(id, job2);

	update_page_annotations(page);

	auto annota_bitmaps = pimpl->m_annotationBitmaps.get_write();
	for (size_t i = 0; i < annota_bitmaps->size(); i++) {
		auto& d = annota_bitmaps->at(i);
		if (d.page != page) {
			continue;
		}
		d.dpi = 0.0f;
	}
}
