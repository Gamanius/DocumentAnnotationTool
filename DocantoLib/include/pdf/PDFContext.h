#ifndef _PDFCONTEXT_H_
#define _PDFCONTEXT_H_

#include "../general/Common.h"
#include "../general/ThreadSafeWrapper.h"

struct fz_context;

namespace Docanto {
	class PDF;

	class PDFContext : public Docanto::ThreadSafeWrapper<fz_context*> {
		PDFContext();
		~PDFContext();

	public:
		PDFContext(const PDFContext&) = delete;
		PDFContext& operator=(const PDFContext&) = delete;

		static PDFContext& get_instance();
	};
}

#endif // !_PDFCONTEXT_H_