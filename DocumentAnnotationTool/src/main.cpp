#pragma once

#include "helper/include.h"
#include "MainWindow.h"
#include <wininet.h>
#include <DbgHelp.h>

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
	if (hInternet == 0) {
		Logger::error("Failed to check for updates:", " with Windows Error: \"", get_win_msg());
		return;
	}

	const std::wstring url = L"https://api.github.com/repos/Gamanius/DocumentAnnotationTool/releases/latest";
	HINTERNET hConnect = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0); 
	if (hConnect == 0) {
		Logger::error("Failed to check for updates with Windows Error: \"", get_win_msg(), "\"");
		return;
	}

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

LONG WINAPI Win32ExceptionFilter(_In_ struct _EXCEPTION_POINTERS* ExceptionInfo) {
	Logger::error("Abnormal Exception ", ExceptionInfo->ExceptionRecord->ExceptionCode);

	// create location for dump file
	auto path = FileHandler::get_appdata_path() / L"crash"; 
	if (std::filesystem::exists(path) == false) {
		std::filesystem::create_directories(path);
	}
	auto file_path = path / L"dump.dmp";

	HANDLE file = CreateFile(file_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		Logger::error(L"Couldn't dump program");
	}

	// Initialize the minidump information
	MINIDUMP_EXCEPTION_INFORMATION mdei;
	mdei.ThreadId = GetCurrentThreadId();
	mdei.ExceptionPointers = ExceptionInfo; 
	mdei.ClientPointers = FALSE;

	// Write the dump
	BOOL success = MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		file,
	    (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithThreadInfo),  
		(ExceptionInfo ? &mdei : NULL),
		NULL,
		NULL
	);

	if (success) {
		Logger::log(L"Dumped core at ", file_path);
	}
	else {
		Logger::error(L"Couldn't dump program");
	}
	CloseHandle(file);

	// call the callback
	if (ABNORMAL_PROGRAM_EXIT_CALLBACK) {
		ABNORMAL_PROGRAM_EXIT_CALLBACK();
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void init() {
	// this is used for memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	AttachConsole(ATTACH_PARENT_PROCESS);
	Logger::unformated_log("---=== Starting Logging ===---");

	SetUnhandledExceptionFilter(Win32ExceptionFilter);

	// load in all settings
	AppVariables::load();

	// Logging
	create_log_folder();
	remove_oldest_log_file();

	Logger::set_handle(create_log_file());
	Logger::write_to_handle();	

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

	// free any possible console that might be attached to it
	auto b = FreeConsole();
	return 0;
}