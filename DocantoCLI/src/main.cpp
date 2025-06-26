#include <iostream>

#include "DocantoLib.h"

using namespace Docanto;
ReadWriteThreadSafeMutex<int> a(1);

void test() {
	auto it = a.get_write();
	std::cout << *it << std::endl;
	*it += 1;
}

int main() {
	Logger::init();
	
	std::thread t(test);
	std::thread t2(test);

	t.join();
	t2.join();

	Logger::print_to_debug();
}