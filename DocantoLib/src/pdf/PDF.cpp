#include "pdf.h"
#include "mupdf/pdf.h"

Docanto::PDF::PDF() {
	fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
}