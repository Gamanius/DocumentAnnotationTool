module Docanto;
import :Logger;

namespace Docanto::Logger {
	void set_out_level(LEVEL lvl) {
		out_level = lvl;
	}
	void set_stream(std::wostream* stream) {
		out_stream = stream;
	}
	std::wostream& get_stream() {
		return *out_stream;
	}

	void start_log(LEVEL lvl) {
		if (print_started)
			return;
		auto& out = get_stream();
		if (use_color)
			switch (lvl) {
			case MSG:
				out << COLOR_WHITE << L"[" << MSG_STRING;
				break;
			case INFO:
				out << COLOR_BLUE << L"[" << INFO_STRING;
				break;
			case SUCCESS:
				out << COLOR_GREEN << L"[" << SUCCESS_STRING;
				break;
			case WARNING:
				out << COLOR_YELLOW << L"[" << WARNING_STRING;
				break;
			case ERROR:
				out << COLOR_RED << L"[" << ERROR_STRING;
				break;

			}
		else 
			switch (lvl) {
			case MSG:
				out  << L"[" << MSG_STRING;
				break;
			case INFO:
				out << L"[" << INFO_STRING;
				break;
			case SUCCESS:
				out << L"[" << SUCCESS_STRING;
				break;
			case WARNING:
				out << L"[" << WARNING_STRING;
				break;
			case ERROR:
				out << L"[" << ERROR_STRING;
				break;

			}
		
		out << L"|";
		get_current_time(out);
		out << L"]: ";
		print_started = true;
	}

	void end_log() {
		if (print_started == false) {
			return;
		}
		auto& out = get_stream();
		out << COLOR_RESET << std::endl;
		print_started = false;
	}

	void use_colors(bool a) {
		use_color = a;
	}
}