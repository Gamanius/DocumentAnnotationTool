#pragma once

#include "helper/include.h"
#include "MainWindow.h"

#define _CRTDBG_MAP_ALLOC

HANDLE create_console() {
	auto b = AllocConsole();
	ASSERT(b, "Could not create Console window");
	return GetStdHandle(STD_OUTPUT_HANDLE);
}

HANDLE create_log_file() {
	auto path = FileHandler::get_appdata_path() / L"docanto.log";
	return CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL); 
}

void init() {
	// this is used for memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	Logger::set_handle(create_log_file());
	Logger::unformated_log("---=== Starting Logging ===---");
	init();

	int argc = 0;
	auto cmdline = CommandLineToArgvW(GetCommandLine(), &argc);
	std::filesystem::path p;

	// possible path
	if (argc > 1) {
		p = cmdline[1]; 
	}
	
	main_window_loop_run(hInstance, p);
	
	Logger::log("End of program");
	return 0;
}