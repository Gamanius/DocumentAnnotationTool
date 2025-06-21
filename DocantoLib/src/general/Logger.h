#include "Common.h"

#include <sstream>

#ifndef _LOGGER_H_
#define _LOGGER_H_

namespace Docanto {
	namespace Logger {
		enum class MSG_LEVEL {
			INFO, SUCCESS, WARNING, ERROR, NONE
		};


		template <typename T>
		concept Streamable = (requires(std::wostream & s, T val) {
			s << val;
		} or std::convertible_to<T, std::string>) and !std::is_enum_v<T>;

		inline std::unique_ptr<std::wostream> _msg_buffer;

		void init(std::wostream* buffer = nullptr);

		template<Streamable S>
		void do_msg(S a) {
			*_msg_buffer << a;
		}

		template<typename T, typename... Targ>
		void do_msg(T first, Targ ... args) {
			do_msg(first);
			do_msg(args ...);
		}

		template<typename T, typename... Targ>
		void _msg(MSG_LEVEL level, T first, Targ... args) {

			switch (level) {
			case MSG_LEVEL::INFO:
				*_msg_buffer << L"[Info]: ";
				break;
			case MSG_LEVEL::WARNING:
				*_msg_buffer << L"[Warning]: ";
				break;
			case MSG_LEVEL::ERROR:
				*_msg_buffer << L"[Error]: ";
				break;
			case MSG_LEVEL::SUCCESS:
				*_msg_buffer << L"[Success]: ";
				break;
			}
			do_msg(first, args ...);

			*_msg_buffer << L"\n";
		}

		template<typename T, typename... Targ>
		void log(T first, Targ... args) {
			_msg(MSG_LEVEL::INFO, first, args...);
		}

		template<typename T, typename... Targ>
		void warn(T first, Targ... args) {
			_msg(MSG_LEVEL::WARNING, first, args...);
		}

		template<typename T, typename... Targ>
		void error(T first, Targ... args) {
			_msg(MSG_LEVEL::ERROR, first, args...);
		}

		template<typename T, typename... Targ>
		void success(T first, Targ... args) {
			_msg(MSG_LEVEL::SUCCESS, first, args...);
		}

		void print_to_debug();
	}
}


#endif // LOGGER_H
