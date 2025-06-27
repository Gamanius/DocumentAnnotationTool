#ifndef _PDF_H_
#define _PDF_H_

#include "../general/Common.h"
#include "../general/MathHelper.h"
#include "../general/File.h"
#include "../general/ThreadSafeWrapper.h"
#include "PDFContext.h"

struct fz_document;
struct fz_page;

namespace Docanto {
	typedef ThreadSafeWrapper<fz_page*> PageWrapper;

	class PDF : public File, public ThreadSafeWrapper<fz_document*> {
		std::vector<std::unique_ptr<PageWrapper>> m_pages;
	public:
		PDF(const std::filesystem::path& p);
		~PDF();

		size_t get_page_count();
		PageWrapper& get_page(size_t page);
		Geometry::Dimension<float> get_page_dimension(size_t page);
	};
}

#endif