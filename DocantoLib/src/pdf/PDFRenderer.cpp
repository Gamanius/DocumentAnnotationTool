#include "PDFRenderer.h"

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

#include "../../include/general/Timer.h"
#include "../../include/general/ReadWriteMutex.h"

#include <unordered_set>

#define FLOAT_EQUAL(a, b) (std::abs(a - b) < 0.001)

struct pair_hash {
	inline std::size_t operator()(const std::pair<size_t, size_t>& v) const {
		return v.first * 31 + v.second;
	}
};

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

	struct RenderJob {
		JobType job = JobType::RENDER_BITMAP;
		size_t page = 0;
		size_t callback_id = 0;

		// For rendering jobs these information are needed
		PDFRenderInfo info;
		Geometry::Rectangle<float> chunk_rec;
		RenderStatus status = RenderStatus::WAITING;
		fz_cookie cookie = {};

		// The list to copy
		std::shared_ptr<DisplayListWrapper> list = nullptr;
		size_t threads_already_copied = 0;

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

	std::unordered_set<std::pair<size_t, size_t>, pair_hash> m_display_list_cache;

public:
	void add_job(size_t id, std::shared_ptr<RenderJob> job) {
		if (job->job == JobType::LOAD_DISPLAY_LIST ) {
			m_display_list_cache.insert({ id, job->page });
		}
		else if (job->job == JobType::DELETE_DISPLAY_LIST) {
			if (m_display_list_cache.contains({ id, job->page })) {
				m_display_list_cache.erase({ id, job->page });
			}
		}
		std::scoped_lock<std::mutex> lock(m_render_worker_queue_mutex);
		job->callback_id = id;
		m_jobs.push_front(job);
		m_render_worker_queue_condition_var.notify_one();
	}

	void set_callback(size_t id, std::function<void(PDFRenderInfo, Image&&)> f) {
		m_job_callback[id] = f;
	}
	
	bool has_display_list(size_t id, size_t page) {
		return m_display_list_cache.contains({ id, page });
	}

	size_t get_amount_threads() {
		return m_render_worker.size();
	}

	size_t remove_finished_jobs() {
		std::scoped_lock<std::mutex> lock(m_render_worker_queue_mutex);
		auto& queue = m_jobs;
		size_t amount = 0;

		for (auto it = queue.begin(); it != queue.end();) {
			if ((*it)->status == RenderStatus::DONE or (*it)->threads_already_copied == get_amount_threads()) {
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

		std::map<std::pair<size_t, size_t>, fz_display_list*> t_content_list;
		
		Docanto::Logger::log("Initialized render thread in ", start);

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
					if (t_content_list.contains({ job->callback_id, job->page })) {
						continue;
					}
					current_job = job;
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
					if (!t_content_list.contains({ job->callback_id, job->page })) {
						// if it doesnt we continue
						continue;
					}

					job->status = RenderStatus::PROCESSING;
					job->render_id = std::this_thread::get_id();
					// we need to safe the pointer since we cannot retrieve it later without being const
					current_job = job;
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
				auto list = t_content_list[{current_job->callback_id, current_job->page}];
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
				t_content_list[{current_job->callback_id, current_job->page}] = copy_list(*(current_job->list->get().get()));
				lock.lock();
				current_job->threads_already_copied++;
				lock.unlock();
				continue;
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
		fz_clear_pixmap_with_value(ctx, pixmap, 0xff); // for the white background
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

std::pair<float, float> Docanto::PDFRenderer::get_chunk_dpi_bound() {
	size_t scale = std::floor(pimpl->m_current_dpi / MUPDF_DEFAULT_DPI);
	return { scale * MUPDF_DEFAULT_DPI, (scale + 1) * MUPDF_DEFAULT_DPI };
}

std::pair<std::vector<Docanto::Geometry::Rectangle<float>>, float> Docanto::PDFRenderer::get_chunks(size_t page) {
	auto dims = pdf_obj->get_page_dimension(page);
	auto  pos = pimpl->m_page_pos.at(page);

	size_t scale = std::floor(pimpl->m_current_dpi / MUPDF_DEFAULT_DPI);
	size_t amount_cells = 3 * scale + 1;
	Geometry::Dimension<float> cell_dim = { dims.width / amount_cells, dims.height / amount_cells };
	
	// transform the viewport to docspace
	auto doc_space_screen = Docanto::Geometry::Rectangle<float>(pimpl->m_current_viewport.upperleft() - pos, pimpl->m_current_viewport.lowerright() - pos);

	Geometry::Point<size_t> topleft = {
		(size_t)std::clamp(std::floor(doc_space_screen.upperleft().x /  dims.width * amount_cells), 0.0f, (float)amount_cells),
		(size_t)std::clamp(std::floor(doc_space_screen.upperleft().y / dims.height * amount_cells), 0.0f, (float)amount_cells),
	};

	Geometry::Point<size_t> bottomright = {
		(size_t)std::clamp(std::ceil(doc_space_screen.lowerright().x / dims.width * amount_cells), 0.0f, (float)amount_cells),
		(size_t)std::clamp(std::ceil(doc_space_screen.lowerright().y / dims.height * amount_cells), 0.0f, (float)amount_cells),
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

	return { chunks, get_chunk_dpi_bound().second };
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

void Docanto::PDFRenderer::create_preview(float dpi) {
	Docanto::Logger::log("Creating preview");
	Timer t;
	size_t amount_of_pages = pdf_obj->get_page_count();
	auto&  positions       = pimpl->m_page_pos;

	float y = 0;
	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i);
		auto id = thread_manager->m_last_id.fetch_add(1);
		m_processor->processImage(id, get_image(i, dpi));

		auto prevs = pimpl->m_previewbitmaps.get_write();
		prevs->push_back({ id, {positions.at(i), dims}, dpi });

		y += dims.height + 10;
	}
	Docanto::Logger::log("Preview generation and processing took ", t);
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

Docanto::ReadWrapper<std::vector<Docanto::PDFRenderer::PDFRenderInfo>> Docanto::PDFRenderer::draw() {
	return pimpl->m_highDefBitmaps.get_read();
}

void Docanto::PDFRenderer::remove_from_processor(size_t id) {
	auto vec = pimpl->m_highDefBitmaps.get_write();

	std::erase_if(*vec, [id](const auto& obj) {
		return obj.id == id;
	});

	m_processor->deleteImage(id);
}

void Docanto::PDFRenderer::add_to_processor() {
	// TODO
}

size_t Docanto::PDFRenderer::cull_bitmaps() {
	auto vec = pimpl->m_highDefBitmaps.get_write();
	std::vector<size_t> ids_to_delete;
	size_t amount = 0;
	auto [ lower_dpi, higher_dpi ] = get_chunk_dpi_bound();

	for (auto it = vec->rbegin(); it != vec->rend(); it++) {
		auto& item = *it;
		bool intersetcts = item.recs.intersects(pimpl->m_current_viewport);
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

size_t Docanto::PDFRenderer::cull_chunks(std::vector<Geometry::Rectangle<float>>& chunks, size_t page) {
	// TODO we also should look into the queue
	auto bitmaps = pimpl->m_highDefBitmaps.get_read();
	auto render_queue = pimpl->m_jobs.get();

	size_t amount = 0;
	auto position = pimpl->m_page_pos.at(page);

	for (auto it = chunks.begin(); it != chunks.end();) {
		bool del = false;
		auto chunk = *it;

		// loop over all bitmaps to see if there are any which are already the same 
		for (size_t j = 0; j < bitmaps->size(); j++) {
			auto bitma = bitmaps->at(j);

			if (FLOAT_EQUAL(chunk.x + position.x, bitma.recs.x) and FLOAT_EQUAL(chunk.y + position.y, bitma.recs.y) and
				FLOAT_EQUAL(chunk.width, bitma.recs.width) and FLOAT_EQUAL(chunk.width, bitma.recs.width)) {
				del = true;
				goto DONE;
			}
		}

		for (size_t j = 0; j < render_queue->size(); j++) {
			auto queue_item = render_queue->at(j);

			// only consider the queue items which are on the correct page
			if (queue_item->page != page) {
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


void Docanto::PDFRenderer::process_chunks(const std::vector<Geometry::Rectangle<float>>& chunks, size_t page) {
	auto [_, dpi] = get_chunk_dpi_bound();

	auto cont = pimpl->m_page_content.get_read()->at(page).get();
	auto position = pimpl->m_page_pos.at(page);

	for (size_t j = 0; j < chunks.size(); j++) {
		auto img = get_image_from_list(cont, chunks.at(j), dpi);

		auto id = thread_manager->m_last_id.fetch_add(1);
		m_processor->processImage(id, img);

		auto prevs = pimpl->m_highDefBitmaps.get_write();
		prevs->push_back({ id, {position + chunks.at(j).upperleft(),chunks.at(j).dims()}, dpi });
	}
}

size_t Docanto::PDFRenderer::abort_queue_item() {
	auto queue = pimpl->m_jobs.get();
	size_t amount = 0;

	auto [_, dpi] = get_chunk_dpi_bound();

	for (auto it = queue->begin(); it != queue->end(); it++) {
		auto& item = *it;

		if (item->info.recs.intersects(pimpl->m_current_viewport) and // intersection test
			(FLOAT_EQUAL(item->info.dpi, dpi)) /*dpi test*/) {
			continue;
		}

		amount++;
		item->cookie.abort = 1;
	}

	return amount;
}

size_t Docanto::PDFRenderer::remove_finished_queue_item() {
	auto queue = pimpl->m_jobs.get().get();
	std::erase_if(*queue, [](std::shared_ptr<RenderThreadManager::RenderJob> info) -> bool {
		return info->status == RenderThreadManager::RenderStatus::DONE;
	});

	return 0;
}

void Docanto::PDFRenderer::request(Geometry::Rectangle<float> view, float dpi) {
	pimpl->m_current_viewport = view;
	pimpl->m_current_dpi = dpi;

	remove_finished_queue_item();
	abort_queue_item();

	cull_bitmaps();

	size_t amount_of_pages = pdf_obj->get_page_count();
	auto&        positions = pimpl->m_page_pos;


	for (size_t i = 0; i < amount_of_pages; i++) {
		auto dims = pdf_obj->get_page_dimension(i);

		auto pdf_rec = Geometry::Rectangle<float>(positions.at(i), dims);

		// skip pdfs which are currently not visible
		if (!pdf_rec.intersects(pimpl->m_current_viewport)) {
			continue;
		}

		if (!thread_manager->has_display_list(id, i)) {
			auto job = std::make_shared<RenderThreadManager::RenderJob>();
			job->job = RenderThreadManager::JobType::LOAD_DISPLAY_LIST;
			job->page = i;
			job->list = pimpl->m_page_content.get_read()->at(i);

			thread_manager->add_job(id, job);
		}

		auto [chunks, dpi] = get_chunks(i);
		cull_chunks(chunks, i);

		for (auto& chunk_rec : chunks) {
			// add the new chunks to the queue
			auto queue = pimpl->m_jobs.get();

			auto job = std::make_shared<RenderThreadManager::RenderJob>();
			job->chunk_rec = chunk_rec;
			job->page = i;
			job->status = RenderThreadManager::RenderStatus::WAITING;

			job->info.dpi = dpi;
			job->info.id = thread_manager->m_last_id.fetch_add(1);
			job->info.recs = { positions.at(i) + chunk_rec.upperleft(), chunk_rec.dims()};

			job->job = RenderThreadManager::JobType::RENDER_BITMAP;

			queue->push_front(job);
			thread_manager->add_job(id, job);
		}
	}
}

void Docanto::PDFRenderer::set_rendercallback(std::function<void(size_t)> fun) { m_render_callback = fun; }

//void Docanto::PDFRenderer::async_render() {
//	Timer start;
//	fz_context* ctx;
//	{
//		auto c = GlobalPDFContext::get_instance().get();
//		ctx = fz_clone_context(*c);
//	}
//
//	auto copy_list = [ctx](fz_display_list* list) {
//		fz_display_list* ls = fz_new_display_list(ctx, (list)->mediabox);
//		ls->len = (list)->len;
//		ls->max = (list)->max;
//		ls->list = new fz_display_node[ls->max];
//		memcpy(ls->list, (list)->list, (list)->len * sizeof(fz_display_node));
//		return ls;
//		};
//
//	auto delete_list = [ctx](fz_display_list* list) {
//		delete[] list->list;
//		list->list = nullptr;
//		list->len = 0;
//		list->max = 0;
//		fz_drop_display_list(ctx, list);
//		};
//
//	std::vector<fz_display_list*> t_content_list;
//
//	// copy all display lists
//	{
//		auto cont = pimpl->m_page_content.get_read();
//		
//		for (size_t i = 0; i < cont->size(); i++) {
//			t_content_list.push_back(
//				copy_list(*(cont->at(i).get()->get().get()))
//			);
//		}
//	}
//
//	Docanto::Logger::log("Initialized render thread in ", start);
//	while (!(pimpl->m_should_worker_die)) {
//		// wait until a signal is received
//		std::unique_lock<std::mutex> lock(pimpl->m_render_worker_queue_mutex);
//		pimpl->m_render_worker_queue_condition_var.wait(lock, [this] {
//			return pimpl->m_should_worker_die or pimpl->m_jobs.size() != 0;
//			});
//
//		// get any item from the render queue
//		std::shared_ptr<impl::RenderJob> current_job = nullptr;
//		{
//			auto& queue = pimpl->m_jobs;
//			for (size_t i = 0; i < queue.size(); i++) {
//				auto& job = queue.at(i);
//				
//				// "accept" canceld jobs so we can remove them
//				if (job->status == impl::RenderStatus::CANCELD) {
//					job->status = impl::RenderStatus::DONE;
//				} 
//				// accept any jobs that are currently in the queue
//				else if (job->status == impl::RenderStatus::WAITING) {
//					job->status = impl::RenderStatus::PROCESSING;
//					job->render_id = std::this_thread::get_id();
//					// we need to safe the pointer since we cannot retrieve it later without being const
//					current_job = job;
//					break;
//				}
//			}
//		}
//
//		// unlock again
//		lock.unlock();
//
//		// we didnt find any rendering jobs and we can go back to waiting
//		if (current_job == nullptr) {
//			continue;
//		}
//
//		auto cont_img = get_image_from_list(ctx, t_content_list[current_job->page], current_job->chunk_rec, current_job->info.dpi, &(current_job->cookie));
//		
//		// we have to check if the rendering was aborted
//		if (current_job->cookie.abort or current_job->status == impl::RenderStatus::CANCELD) {
//			current_job->status = impl::RenderStatus::DONE;
//			continue;
//		}
//
//		// add the new image to the list
//		m_processor->processImage(current_job->info.id, cont_img);
//		pimpl->m_highDefBitmaps.get_write()->push_back(current_job->info);
//
//		// call the callback
//		if (m_render_callback)
//			m_render_callback(current_job->info.id);
//
//		current_job->status = impl::RenderStatus::DONE;
//	}
//
//	for (size_t i = 0; i < t_content_list.size(); i++) {
//		delete_list(t_content_list[i]);
//	}
//
//	fz_drop_context(ctx);
//}

void Docanto::PDFRenderer::receive_image(PDFRenderInfo info, Image&& i) {
	// add the new image to the list
	m_processor->processImage(info.id, i);
	pimpl->m_highDefBitmaps.get_write()->push_back(info);

	// call the callback
	if (m_render_callback)
		m_render_callback(info.id);
}

void Docanto::PDFRenderer::render() {
	//auto queue = pimpl->m_jobs.get_write();
	//auto job = queue->back();
	//queue->pop_back();

	//job.status = impl::RenderStatus::PROCESSING;

	//auto& cont = pimpl->m_page_content.get_read()->at(job.page);
	//auto img = get_image_from_list(cont.get(), job.chunk_rec, job.info.dpi);

	//m_processor->processImage(job.info.id, img);
	//pimpl->m_highDefBitmaps.get_write()->push_back(job.info);	
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
		render->draw_rect(info.recs, { 0, 0, 255 });
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
		auto p = fz_load_page(*ctx, *doc, static_cast<int>(i));
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