#include "pdf.h"
#include "mupdf/pdf.h"


void lock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].lock();
}

void unlock_mutex(void* user, int lock) {
	((std::mutex*)user)[lock].unlock();
}

Docanto::PDF::PDF() {
	fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
}