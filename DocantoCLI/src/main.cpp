#include <iostream>

#include "DocantoLib.h"

using namespace Docanto;
ReadWriteThreadSafeMutex<int> a(1);

void write_test() {
	auto it = a.get_write();
	auto it2 = a.get_read();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << *it << std::endl;
	*it += 1;
}

void read_test() {
	auto it = a.get_read();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << *it << std::endl;
}

int main() {
	Logger::init();
	
	std::thread t(read_test);
	std::thread t3(write_test);
	std::thread t4(write_test);
	std::thread t5(write_test);
	std::thread t2(read_test);

	t.join();
	t2.join();
	t3.join();
	t5.join();
	t4.join();

	Logger::print_to_debug();
}