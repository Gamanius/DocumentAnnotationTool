#include <iostream>

#include "DocantoLib.h"

int main() {
	using namespace Docanto;
	Logger::init();
	
	Timer t;

	Logger::log(t);

	Logger::print_to_debug();
}