#pragma once

#include "helper/include.h"
#include "MainWindow.h"
#include <wininet.h>

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

void check_updates() {
	// Check in GitHub if there is a new version available
	HINTERNET hInternet = InternetOpen(L"GitHubReleaseChecker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	ASSERT_WIN_WITH_STATEMENT(hInternet != 0, return, "Failed to check for updates:");

	const std::wstring url = L"https://api.github.com/repos/Gamanius/DocumentAnnotationTool/releases/latest";
	HINTERNET hConnect = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0); 
	ASSERT_WIN_WITH_STATEMENT(hConnect != 0, return, "Failed to check for updates:");

	char buffer[1024];
	DWORD bytesRead;
	std::string response;

	while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) and bytesRead != 0) {
		response.append(buffer, bytesRead);
	}

	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	using json = nlohmann::json;
	json j;
	try {
		j = json::parse(response);
		std::string tag = j.at("tag_name").get<std::string>(); 
		if (tag != APPLICATION_VERSION) {
			Logger::log("New version available: ", tag);
			auto msg = MessageBox(NULL, L"New version available!\nOpen Download link?", L"Docanto", MB_YESNO | MB_ICONINFORMATION);
			if (msg == IDYES) {
				ShellExecute(NULL, L"open", L"https://github.com/Gamanius/DocumentAnnotationTool/releases/latest", NULL, NULL, SW_SHOWNORMAL);
			}
		}
		else {
			Logger::log("No new version available");
		}
	}
	catch (json::exception& e) {
		Logger::error("JSON error when checking for updated: ", e.what()); 
	}
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
	Logger::unformated_log("---=== Starting Logging ===---");

	// check for updates
	check_updates();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
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