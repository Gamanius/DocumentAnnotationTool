#pragma once

#include "helper/include.h"
#include "MainWindow.h"

#define _CRTDBG_MAP_ALLOC

HANDLE create_console() {
	auto b = AllocConsole();
	ASSERT(b, "Could not create Console window");
	return GetStdHandle(STD_OUTPUT_HANDLE);
}

void init() {
	// this is used for memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	//Logger::set_console_handle(create_console());
	init();
	Logger::log("");
	auto out = FileHandler::open_file_dialog(L"Bitmaps (*.bmp)\0*.bmp\0\0");
	if (out.has_value()) {
		auto file = FileHandler::open_file(out.value());
	}
	//main_window_loop_run(hInstance);
	
	Logger::print_to_debug();
	Logger::clear();
}