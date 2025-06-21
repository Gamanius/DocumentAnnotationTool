#include <iostream>

#include "DocantoLib.h"

int main() {
	using namespace Docanto;
	Logger::init();
	
	Geometry::Point<int> p(1, 1);
	Geometry::Rectangle<int> a(1, 1, 2, 3);
	Logger::log(p, a);

	Logger::print_to_debug();
}