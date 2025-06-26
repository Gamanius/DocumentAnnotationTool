#include <iostream>

#include "DocantoLib.h"

using namespace Docanto;

int main() {
	Logger::init(&std::wcout);

	auto& t = PDFContext::get_instance();

	Logger::print_to_debug();
}