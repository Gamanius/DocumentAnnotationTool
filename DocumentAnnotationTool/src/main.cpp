#pragma once

#include "helper/include.h"
#include "MainWindow.h"

#define _CRTDBG_MAP_ALLOC

void create_log_folder() {
	auto path = FileHandler::get_appdata_path() / L"log";
	if (!std::filesystem::exists(path)) {
		std::filesystem::create_directory(path);
	}
}

void remove_oldest_log_file() {
	auto path = FileHandler::get_appdata_path() / L"log"; 
	std::filesystem::path oldestFile; 
	auto oldestTime = std::filesystem::file_time_type::clock::now();
	size_t counter = 0;

	for (const auto& entry : std::filesystem::directory_iterator(path)) { 
		if (!std::filesystem::is_regular_file(entry)) {
			continue;
		}
		counter++;
		auto currentFileTime = std::filesystem::last_write_time(entry);

		// If oldestFile is uninitialized or the current file is older, update oldestFile
		if (oldestFile.empty() or currentFileTime < oldestTime) {
			oldestTime = currentFileTime;
			oldestFile = entry.path();
		}
	}

	if (counter < AppVariables::APPVARIABLES_MAX_LOG_FILES) {
		return;
	}

	if (!oldestFile.empty()) 
		std::filesystem::remove(oldestFile);
}

HANDLE create_console() {
	auto b = AllocConsole();
	ASSERT(b, "Could not create Console window");
	return GetStdHandle(STD_OUTPUT_HANDLE);
}

HANDLE create_log_file() {
	std::wstringstream w;
	w << L"docanto ";
	Logger::get_current_time(w);
	w << L".log";
	auto s = w.str();
	std::replace(s.begin(), s.end(), L':', L'-'); 

	auto path = FileHandler::get_appdata_path() / L"log" / s;
	
	return CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL); 
}

void init() {
	// this is used for memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// load in all settings
	AppVariables::load();

	// Logging
	create_log_folder();
	remove_oldest_log_file();

	Logger::set_handle(create_log_file());

}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	init();
	Logger::unformated_log("---=== Starting Logging ===---");


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