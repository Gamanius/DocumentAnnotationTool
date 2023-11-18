#pragma once

#include "helper/include.h"

#define APPLICATION_NAME L"Docanto"		

void test() {
	auto pls = MemoryWatcher::get_allocated_bytes();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	std::atexit(test);
	MemoryWatcher::set_calibration();
	auto b = AllocConsole();
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	
	auto window = WindowHandler(APPLICATION_NAME, hInstance);
	window.set_state(WindowHandler::WINDOW_STATE::NORMAL);
	
	while (!window.close_request()) { 
		WindowHandler::get_window_messages(true);
	}
	
	Logger::print_to_console(handle);
	Logger::clear();
}