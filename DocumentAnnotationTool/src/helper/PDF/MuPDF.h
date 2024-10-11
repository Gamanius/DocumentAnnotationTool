#include "../General/General.h"
#include "ThreadSafeClass.h"

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

#ifndef _MUPDF_H_
#define _MUPDF_H_

constexpr float MUPDF_DEFAULT_DPI = 72.0f;

typedef RawSafeClass<fz_context*, std::recursive_mutex> ThreadSafeContextWrapper;
struct ContextWrapper : public ThreadSafeClass<fz_context*> {
	ContextWrapper(fz_context* c);

	/// <summary>
	/// alias for get_item
	/// </summary>
	/// <returns></returns>
	ThreadSafeContextWrapper get_context();

	~ContextWrapper();
};

typedef RawSafeClass<fz_page*, std::recursive_mutex> ThreadSafePageWrapper;

struct PageWrapper : public ThreadSafeClass<fz_page*> {
private:
	std::shared_ptr<ContextWrapper> m_context;
public:
	PageWrapper(std::shared_ptr<ContextWrapper> a, fz_page* p);

	ThreadSafePageWrapper get_page();

	~PageWrapper();
};

struct DocumentWrapper;
typedef RawSafeClass<fz_document*, std::recursive_mutex> ThreadSafeDocumentWrapper;

struct DocumentWrapper : public ThreadSafeClass<fz_document*> {
private:
	std::shared_ptr<ContextWrapper> m_context;
public:
	DocumentWrapper(std::shared_ptr<ContextWrapper> a, fz_document* d);

	ThreadSafeDocumentWrapper get_document();

	~DocumentWrapper();
};

struct DisplayListWrapper;
typedef RawSafeClass<fz_display_list*, std::recursive_mutex> ThreadSafeDisplayListWrapper;

struct DisplayListWrapper : public ThreadSafeClass<fz_display_list*> {
private:
	std::shared_ptr<ContextWrapper> m_context;
public:
	DisplayListWrapper(std::shared_ptr<ContextWrapper> a, fz_display_list* l);

	ThreadSafeDisplayListWrapper get_displaylist();

	~DisplayListWrapper();
};


class MuPDFHandler {
	std::shared_ptr<ContextWrapper> m_context;

	std::mutex m_mutex[FZ_LOCK_MAX];
	fz_locks_context m_locks;

	static void lock_mutex(void* user, int lock);
	static void unlock_mutex(void* user, int lock);
	static void error_callback(void* user, const char* message);
	static void warning_callback(void* user, const char* message);
public:
	struct DisplaylistChangeInfo {
		enum CHANGE_TYPE {
			SWITCH,
			ADDITION,
			REMOVAL,
			UNKNOWN
		};

		struct DisplaylistChangeType {
			CHANGE_TYPE type = CHANGE_TYPE::UNKNOWN;
			int page1 = 0;
			int page2 = 0;
		};

		std::vector<DisplaylistChangeType> page_info;
	};

	struct PDF : public FileHandler::File {
	private:
		// Pointer to the context from the MuPDFHandler
		std::shared_ptr<ContextWrapper> m_ctx;
		std::shared_ptr<DocumentWrapper> m_document;
		// TODO
		std::vector<std::shared_ptr<PageWrapper>> m_pages;

		// Position and dimensions of the pages 
		std::shared_ptr<ThreadSafeVector<Math::Rectangle<float>>> m_pagerec;

	public:

		PDF() = default;
		PDF(std::shared_ptr<ContextWrapper> ctx, std::shared_ptr<DocumentWrapper> doc);

		// move constructor
		PDF(PDF&& t) noexcept;
		// move assignment
		PDF& operator=(PDF&& t) noexcept;

		// again dont copy
		PDF(const PDF& t) = delete;
		PDF& operator=(const PDF& t) = delete;

		// the fz_context is not allowed to be droped here
		~PDF();

		ThreadSafeContextWrapper get_context() const;

		ThreadSafeDocumentWrapper get_document() const;

		std::shared_ptr<ThreadSafeVector<Math::Rectangle<float>>> get_pagerec();

		/// <summary>
		/// The document HAS TO outlive the page!! 
		/// TODO put fz_pages into DocumentWrapper
		/// </summary>
		/// <param name="doc"></param>
		/// <param name="page"></param>
		/// <returns></returns>
		ThreadSafePageWrapper get_page(size_t page);

		void sort_page_positions();

		std::shared_ptr<ContextWrapper> get_context_wrapper() const;

		/// <summary>
		/// Retrieves the number of pdf pages
		/// </summary>
		/// <returns>Number of pages</returns>
		size_t get_page_count() const;

		void save_pdf(const std::wstring& path);
		void add_page(PDF& pdf, int page = INT_MAX);

		Math::Rectangle<float> get_page_size(size_t page, float dpi = 72);
		Math::Rectangle<float> get_bounds() const;
	};

	MuPDFHandler();

	std::optional<PDF> load_pdf(const std::wstring& path);

	ThreadSafeContextWrapper get_context();

	~MuPDFHandler();
};

#endif // !_MUPDF_H_
