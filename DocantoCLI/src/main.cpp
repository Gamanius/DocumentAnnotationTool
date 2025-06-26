#include <iostream>

#include "DocantoLib.h"

using namespace Docanto;
ThreadSafeWrapper<int> a(1);

void get() {
	auto t = a.try_get();
	Logger::log(t);
}


int main() {
	Logger::init(&std::wcout);

	std::thread a(get);
	std::thread a2(get);

	a.join();
	a2.join();
	Logger::log("dewa");
	Logger::print_to_debug();
}