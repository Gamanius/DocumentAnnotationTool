#pragma once

#include <Windows.h>
#include <string>
#include <vector>

namespace Logger {
	enum MsgLevel {
		INFO = 0,
		WARNING = 1,
		FATAL = 2
	};

	void log(const std::wstring& msg, MsgLevel lvl = MsgLevel::INFO);
	void log(const std::string& msg, MsgLevel lvl = MsgLevel::INFO);

	void assert_msg(const std::string& msg, const std::string& file, long line);
	
	void print_to_console(HANDLE handle);

	const std::vector<std::wstring>* get_all_msg();
}

#ifndef NDEBUG
#define ASSERT(x, y) if (!(x)) { Logger::assert_msg(y, __FILE__, __LINE__); __debugbreak(); }
#else
#define ASSERT(x, y)
#endif // !NDEBUG
