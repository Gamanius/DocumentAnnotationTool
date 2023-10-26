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

	void assert_msg(const std::string& msg, const std::string& file, long line) {
		log("Assert failed in File: " + file + "on line " + std::to_string(line) + " with message: \"" + msg + "\" ", Logger::MsgLevel::FATAL);
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

	const std::vector<std::wstring>* get_all_msg() {
		return &all_messages;
	}


}