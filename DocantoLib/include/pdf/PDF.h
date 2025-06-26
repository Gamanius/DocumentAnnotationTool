#ifndef _PDF_H_
#define _PDF_H_

#include "../general/Common.h"
#include "../general/File.h"
#include "PDFContext.h"


namespace Docanto {
	class PDF : public File {
	public:
		PDF(const std::filesystem::path& p);
		~PDF();
	};
}

#endif