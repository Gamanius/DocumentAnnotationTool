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
		concept Streamable = (requires(std::wostream & s, const T& val) {
			s << val;
		} or std::convertible_to<T, std::string>) and !std::is_enum_v<T>;

		template <class T>
		concept StreamableOptional = requires(const T& t) {
			t.value();
			t.has_value();
		};

		inline std::shared_ptr<std::wostream> _internal_buffer;
		inline std::wostream* _msg_buffer;
		inline std::mutex     _msg_mutex;

		void init(std::wostream* buffer = nullptr);

		template<Streamable S>
		void do_msg(const S& a) {
			*_msg_buffer << a;
		}

		template<StreamableOptional S>
		void do_msg(const S& a) {
			if (a.has_value()) {
				*_msg_buffer << a.value();
			}
			else {
				*_msg_buffer << L"std::nullopt";
			}
		}

		template<typename T, typename... Targ>
		void do_msg(const T& first, const Targ& ... args) {
			do_msg(first);
			do_msg(args ...);
		}

		template<typename T, typename... Targ>
		void _msg(MSG_LEVEL level, const T& first, const Targ&... args) {
			std::scoped_lock<std::mutex> lock(_msg_mutex);
			auto id = std::this_thread::get_id();

			if (id != MAIN_THREAD_ID) {
				*_msg_buffer << L"{" << id << L"}";
			}

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
		void log(const T& first, const Targ&... args) {
			_msg(MSG_LEVEL::INFO, first, args...);
		}

		template<typename T, typename... Targ>
		void warn(const T& first, const Targ&... args) {
			_msg(MSG_LEVEL::WARNING, first, args...);
		}

		template<typename T, typename... Targ>
		void error(const T& first, const Targ&... args) {
			_msg(MSG_LEVEL::ERROR, first, args...);
		}

		template<typename T, typename... Targ>
		void success(const T& first, const Targ&... args) {
			_msg(MSG_LEVEL::SUCCESS, first, args...);
		}

		void print_to_debug();
	}
}


#endif // LOGGER_H
