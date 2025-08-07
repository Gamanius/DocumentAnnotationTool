#ifndef _GLOBALPDFCONTEXT_H_
#define _GLOBALPDFCONTEXT_H_

#include "general/Common.h"
#include "general/ThreadSafeWrapper.h"

inline constexpr float MUPDF_DEFAULT_DPI = 72.0f;

struct fz_context;

namespace Docanto {
	class PDF;

	class GlobalPDFContext : public Docanto::ThreadSafeWrapper<fz_context*> {
		GlobalPDFContext();
		~GlobalPDFContext();

	public:
		GlobalPDFContext(const GlobalPDFContext&) = delete;
		GlobalPDFContext& operator=(const GlobalPDFContext&) = delete;

		static GlobalPDFContext& get_instance();
	};
}

#endif // !_PDFCONTEXT_H_