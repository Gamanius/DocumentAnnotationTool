#pragma once

#include "include.h"
#include <locale>
#include <codecvt>

namespace Logger {
	std::vector<std::wstring> all_messages;

	void log(const std::wstring& msg, MsgLevel lvl) {
		all_messages.push_back(msg);
	}

	void log(const std::string& msg, MsgLevel lvl) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(msg);
		log(wide, lvl);
	}

	void log(const unsigned long msg, MsgLevel lvl) {
		log(std::to_string(msg), lvl);
	}

	void warn(const std::string& msg) {
		log(msg, Logger::MsgLevel::WARNING);
	}

	void assert_msg(const std::string& msg, const std::string& file, long line) {
		log("Assert failed in File: " + file + " on line " + std::to_string(line) + " with message: \"" + msg + "\" ", Logger::MsgLevel::FATAL);
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

	void print_to_console(HANDLE handle) {
		// TODO actually awfully slow 
		const wchar_t* new_line = L"\n";

		for (size_t i = 0; i < all_messages.size(); i++) {
			std::wstring& temp = all_messages.at(i);
			WriteConsoleW(handle, (void*)temp.c_str(), temp.length(), nullptr, nullptr);
			WriteConsoleW(handle, (void*)new_line, 1, nullptr, nullptr);
		}
	}

	void clear() {
		all_messages.clear();
	}

	const std::vector<std::wstring>* get_all_msg() {
		return &all_messages;
	}
}