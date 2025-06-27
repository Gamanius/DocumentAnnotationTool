#ifndef _PDF_H_
#define _PDF_H_

#include "../general/Common.h"
#include "../general/File.h"
#include "../general/ThreadSafeWrapper.h"
#include "PDFContext.h"

struct fz_document;

namespace Docanto {
	class PDF : public File, public ThreadSafeWrapper<fz_document*> {
	public:
		PDF(const std::filesystem::path& p);
		~PDF();

		size_t get_page_count();
	};
}

#endif