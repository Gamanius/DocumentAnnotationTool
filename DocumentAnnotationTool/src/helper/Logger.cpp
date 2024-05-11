#pragma once

#include "include.h"
#include <locale>
#include <codecvt>

std::shared_ptr<std::vector<std::wstring>> all_messages = std::make_shared<std::vector<std::wstring>>();
HANDLE consoleHandle = nullptr;

namespace Logger {

	void log(const std::wstring& msg, MsgLevel lvl) {
		all_messages->push_back(msg);
	}

	void log(const std::string& msg, MsgLevel lvl) {
		// could be unsecure
		auto size = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, NULL, 0);
		std::wstring wide(size, 0); 
		MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, &wide[0], size);
		log(wide, lvl); 
	}

	void log(const unsigned long msg, MsgLevel lvl) {
		log(std::to_string(msg), lvl);
	}

	void log(int msg, MsgLevel lvl) {
		log(std::to_string(msg), lvl);
	}

	void log(size_t msg, MsgLevel lvl) {
		log(std::to_string(msg), lvl);
	}

	void log(const double msg, MsgLevel lvl) {
		log(std::to_string(msg), lvl);
	}

	void warn(const std::string& msg) {
		log(msg, Logger::MsgLevel::WARNING);
	}

	void assert_msg(const std::string& msg, const std::string& file, long line) {
		log("Assert failed in File: " + file + " on line " + std::to_string(line) + 
			" with message: \"" + msg + "\" ", Logger::MsgLevel::FATAL);

		print_to_debug(); 
	}

	void assert_msg_win(const std::string& msg, const std::string& file, long line) {
		// Get the last error code
		DWORD error = GetLastError();

		// Use FormatMessage to get a human-readable error message
		LPVOID errorMsg;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			error,
			0, // Default language
			(LPWSTR)&errorMsg,
			0,
			NULL
		);

		log(L"Windows Error (" + std::to_wstring(error) + L"): " + std::wstring((wchar_t*)errorMsg));
		assert_msg(msg, file, line);
	}

	void assert_msg(const std::wstring& msg, const std::string& file, long line) {
		// could be unsecure
		auto size = MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, NULL, 0);
		std::wstring wide(size, 0);
		MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, &wide[0], size);

		log(L"Assert failed in File: " + wide + L" on line " + std::to_wstring(line) +  
			L" with message: \"" + msg + L"\" ", Logger::MsgLevel::FATAL); 
	}

	void assert_msg_win(const std::wstring& msg, const std::string& file, long line) {
		// Get the last error code
		DWORD error = GetLastError();

		// Use FormatMessage to get a human-readable error message
		LPVOID errorMsg;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			error,
			0, // Default language
			(LPWSTR)&errorMsg,
			0,
			NULL
		);

		log(L"Windows Error (" + std::to_wstring(error) + L"): " + std::wstring((wchar_t*)errorMsg));

		assert_msg(msg, file, line);
	}

	void set_console_handle(HANDLE handle) {
		consoleHandle = handle;
	}

	void print_to_console(bool clear) {
		if (consoleHandle == nullptr)
			return;

		// TODO actually awfully slow 
		const wchar_t* new_line = L"\n";

		for (size_t i = 0; i < all_messages->size(); i++) {
			std::wstring& temp = all_messages->at(i);
			WriteConsoleW(consoleHandle, (void*)temp.c_str(), (DWORD)temp.length(), nullptr, nullptr);
			WriteConsoleW(consoleHandle, (void*)new_line, 1, nullptr, nullptr);
		}
		if (clear)
			all_messages->clear();
	}

	void print_to_debug(bool clear) {
		for (size_t i = 0; i < all_messages->size(); i++) {
			OutputDebugString(L"\n");
			OutputDebugString(all_messages->at(i).c_str());
		}
		OutputDebugString(L"\n\n");
		if (clear)
			all_messages->clear();
	}

	void clear() {
		all_messages->clear();
	}

	const std::weak_ptr<std::vector<std::wstring>> get_all_msg() {
		return all_messages;
	}
}