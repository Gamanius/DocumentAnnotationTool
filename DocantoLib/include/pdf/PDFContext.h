#include "../general/Common.h"

struct fz_context;

namespace Docanto {

	class PDFContext {
		PDFContext();
		~PDFContext();


		struct Impl;

		std::unique_ptr<Impl> m_impl;

	public:
		PDFContext(const PDFContext&) = delete;
		PDFContext& operator=(const PDFContext&) = delete;

		static PDFContext& get_instance();

		fz_context* get_context() const;

	};
}