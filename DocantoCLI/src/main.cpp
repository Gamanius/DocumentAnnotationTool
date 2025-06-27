#include <iostream>

#include "DocantoLib.h"

using namespace Docanto;

int main() {
	Logger::init(&std::wcout);

	PDF a("C:/repos/Docanto/pdf_tests/Annotation pdf.pdf");

	Logger::log(a.get_page_count());

	Logger::print_to_debug();
}