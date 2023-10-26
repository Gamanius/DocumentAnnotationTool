#include "helper/include.h"
#include <iostream>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {

	auto b = AllocConsole();
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);

	Logger::log("Hello world");
	ASSERT(false, "oh no")
	Logger::print_to_console(handle);
}