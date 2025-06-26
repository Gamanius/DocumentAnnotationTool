#include <iostream>

#include "DocantoLib.h"

using namespace Docanto;
ThreadSafeWrapper<int> a(1);

void get() {
	auto t = a.try_get();
	auto id = std::this_thread::get_id();
	std::cout << id << std::endl;
	if (t.has_value())
	{
		std::cout << *t.value() << std::endl;
	}
}

void try_get() {

}

int main() {
	Logger::init();

	std::thread t(get);
	std::thread t2(get);
	std::thread t3(get);

	t.join();
	t2.join();
	t3.join();

	Logger::print_to_debug();
}