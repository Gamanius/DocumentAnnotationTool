export module Docanto:Logger;

import <string>;
import <time.h>;
import <iostream>;
import <chrono>;

namespace Docanto::Logger {
	// set standard stream to (w)cout
	std::wostream* out_stream = &std::wcout;
	bool use_color = true;

	export const std::wstring COLOR_RESET = L"\033[0m";
	export const std::wstring COLOR_WHITE = L"\033[37m";
	export const std::wstring COLOR_BLUE = L"\033[34m";
	export const std::wstring COLOR_GREEN = L"\033[32m";
	export const std::wstring COLOR_YELLOW = L"\033[33m";
	export const std::wstring COLOR_RED = L"\033[31m";

	const std::wstring MSG_STRING     = L"Message";
	const std::wstring INFO_STRING    = L"Info";
	const std::wstring SUCCESS_STRING = L"Success";
	const std::wstring WARNING_STRING = L"Warning";
	const std::wstring ERROR_STRING   = L"Error";

	export enum LEVEL {
		MSG = 0,
		INFO = 1,
		SUCCESS = 2,
		WARNING = 3,
		ERROR = 4,
	};

	template <typename T>
	concept Streamable = (requires(std::wstringstream & s, T val) {
		s << val;
	} or std::convertible_to<T, std::string>) and !std::is_enum_v<T>;

	// Time
	void get_current_time(std::wostream& s) {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		auto sec   = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()) % 60;
		auto min   = std::chrono::duration_cast<std::chrono::minutes>(now.time_since_epoch()) % 60;
		auto hour  = std::chrono::duration_cast<std::chrono::hours>(now.time_since_epoch()) % 24;

		s << std::setfill(L'0') << std::setw(2) << hour.count() << L':'
		                        << std::setw(2) << min.count() << L':'
								<< std::setw(2) << sec.count() << L':'
								<< std::setw(3) << milli.count();
	}

	export void set_stream(std::wostream* stream);
	export std::wostream& get_stream();

	export void use_colors(bool a);

	void start_log(LEVEL lvl = LEVEL::MSG);
	void end_log();

	export template<Streamable T>
	void log(T a, LEVEL lvl = LEVEL::MSG) {
		start_log(lvl);
		auto& out = get_stream();
		out << a;
		end_log();
	}

}