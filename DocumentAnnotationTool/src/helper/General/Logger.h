#pragma once

#include <sstream>
#include <mutex>
#include <Windows.h>
#include <optional>
#include <chrono>

#include "MagicEnum.h"

#ifndef _LOGGER_H_
#define _LOGGER_H_


inline std::wstring get_win_msg() {
	auto error = GetLastError();
	LPWSTR buffer = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error, LANG_SYSTEM_DEFAULT, (LPWSTR)&buffer, 0, NULL);
	if (buffer == nullptr) {
		return std::to_wstring(error);
	}
	std::wstring msg = buffer;
	LocalFree(buffer); // Free the buffer.
	// remove newline and carriage return
	msg = std::wstring(msg.begin(), msg.end() - 2);
	// also return the error code
	return L"(" + std::to_wstring(error) + L") " + msg;
}

namespace Logger {
	enum MSG_LEVEL {
		_INFO = 0,
		_WARNING = 1,
		_ERROR = 2,
		_SUCCESS = 3,
		_NONE = 4
	};

	inline std::wstringstream process_msg_stream;
	inline std::wstringstream intermediate_stream;
	inline std::recursive_mutex log_mutex;

	inline HANDLE io_handle = nullptr;
	inline bool handle_supports_color = false;

	const std::wstring info_string = L"[Info|";
	const std::wstring info_color = L"[\033[36mInfo\033[0m|";

	const std::wstring warning_string = L"[Warning|";
	const std::wstring warning_color = L"[\033[33mWarning\033[0m|";

	const std::wstring error_string = L"[Error|";
	const std::wstring error_color = L"[\033[31mError\033[0m|";

	const std::wstring success_string = L"[Success|";
	const std::wstring success_color = L"[\033[32mSuccess\033[0m|";


	template <typename T>
	concept Streamable = (requires(std::wstringstream & s, T val) {
		s << val;
	} or std::convertible_to<T, std::string>) and !std::is_enum_v<T>;

	template <typename T>
	concept StreamableContainer = requires(T arr) {
		{
			arr.begin()
		} -> std::same_as<typename T::iterator>;
		{
			arr.end()
		} -> std::same_as<typename T::iterator>;
		{
			arr.size()
		} -> std::same_as<size_t>;
	} and !std::convertible_to<T, std::wstring> and !std::convertible_to<T, std::string>;

	/// <summary>
	/// https://stackoverflow.com/questions/68443804/c20-concept-to-check-tuple-like-types
	/// </summary>
	template<class T, std::size_t N>
	concept has_tuple_element = requires(T t) {
		typename std::tuple_element_t<N, std::remove_const_t<T>>;
		std::get<N>(t);

	};

	template<class T>
	concept StreamableTuple = !std::is_reference_v<T> and requires(T t) {
		typename std::tuple_size<T>::type;
		requires std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
	} and
		[]<std::size_t... N>(std::index_sequence<N...>) {
		return (has_tuple_element<T, N> and ...);
	} (std::make_index_sequence<std::tuple_size_v<T>>());

	template <class T>
	concept StreamableEnum = std::is_enum_v<T>;

	template <class T>
	concept StreamableOptional = requires(T t) {
		t.value();
		t.has_value();
	} and std::convertible_to<T, std::optional<T>>;

	/// <summary>
	/// Base case
	/// </summary>
	inline void process_msg() {}


	// Weird behaviour: If a string is passed into process_msg it will be considered as a
	// Streamable but if it's inside of a template like e.g. std::pair<T, std::string> it
	// need this function or else it doesn't find anything. I'm really confused by this.
	inline void process_msg(std::string msg) {
		auto size = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, NULL, 0);
		std::wstring wide(size, 0);
		MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), -1, &wide[0], size);
		intermediate_stream << wide;
	}

	template <StreamableEnum T>
	void process_msg(T msg) {
		intermediate_stream << magic_enum::enum_name(msg);
	}

	template <Streamable T>
	void process_msg(T msg) {
		intermediate_stream << msg;
	}

	template <StreamableTuple T>
	void process_msg(T msg);
	template <StreamableOptional T>
	void process_msg(T msg);

	template <StreamableContainer T>
	void process_msg(T arr) {
		size_t size = arr.size();
		intermediate_stream << "[";
		for (const auto& i : arr) {
			process_msg(i);
			intermediate_stream << (--size > 0 ? ", " : "");
		}
		intermediate_stream << "]";
	}

	template <StreamableTuple T>
	void process_msg(T msg) {
		size_t size = std::tuple_size<T>{};
		intermediate_stream << "{";
		std::apply([&](auto&&... args) {
			size_t index = 0;
			((process_msg(args), intermediate_stream << (++index < size ? ", " : "")), ...);
			}, msg);
		intermediate_stream << "}";
	}

	template <StreamableOptional T>
	void process_msg(T msg) {
		if (msg.has_value()) {
			process_msg(msg.value());
		}
		else {
			intermediate_stream << "<std::nullopt>";
		}
	}

	template<typename T, typename... Targ>
	void process_msg(T first, Targ... args) {
		process_msg(first);
		process_msg(args...);
	}

	// Time
	inline void get_current_time(std::wstringstream& s) {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		std::tm* local = new std::tm;
		localtime_s(local, &in_time_t);
		s << std::put_time(local, L"%X") << L":" << std::setfill(L'0') << std::setw(3) << milliseconds.count();
		delete local;
	}


	void print_intermediate_stream(std::wstringstream& s);

	template<typename T, typename... Targ>
	void create_msg(MSG_LEVEL level, T first, Targ... args) {
		// lock the mutex
		std::lock_guard<std::recursive_mutex> lock(log_mutex);
		// First process the messages 
		process_msg(first, args...);
		std::wstringstream temp_stream;

		// get the time and add the string
		switch (level) {
		case Logger::_INFO:
			process_msg_stream << info_string;
			temp_stream << info_string;
			break;
		case Logger::_WARNING:
			process_msg_stream << warning_string;
			temp_stream << warning_string;
			break;
		case Logger::_ERROR:
			process_msg_stream << error_string;
			temp_stream << error_string;
			break;
		case Logger::_SUCCESS:
			process_msg_stream << success_string;
			temp_stream << success_string;
			break;
		case Logger::_NONE:
			goto NO_FORMATING;
			break;
		default:
			break;
		}

		get_current_time(process_msg_stream);
		get_current_time(temp_stream);

		process_msg_stream << "]: ";
		temp_stream << "]: ";

	NO_FORMATING:

		process_msg_stream << intermediate_stream.str() << "\n";
		temp_stream << intermediate_stream.str() << "\n";


		print_intermediate_stream(temp_stream);

		// clear the intermediate stream
		intermediate_stream.str(L"");
	}

	inline void clear_stream() {
		process_msg_stream.str(L"");
	}

	inline void set_handle(HANDLE h, bool add_color = false) {
		io_handle = h;
		handle_supports_color = add_color;
	}

	inline void add_color_to_string(std::wstring& s) {
		auto replaceAll = [&s](const std::wstring what, const std::wstring to) {
			size_t pos = 0;
			while ((pos = s.find(what, pos)) != std::string::npos) {
				s.replace(pos, what.length(), to);
				pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
			}
			};

		replaceAll(info_string, info_color);
		replaceAll(success_string, success_color);
		replaceAll(warning_string, warning_color);
		replaceAll(error_string, error_color);
	}

	inline void write_to_handle(bool clear = false, HANDLE h = nullptr, std::wstringstream& s = process_msg_stream) {
		HANDLE cur_io_handle = h == nullptr ? io_handle : h;
		if (cur_io_handle == nullptr) {
			return;
		}

		if (!handle_supports_color) {
			WriteFile(cur_io_handle, s.str().c_str(), (DWORD)(s.str().size() * sizeof(wchar_t)), NULL, NULL);
		}
		else {
			// Add color
			std::wstring temp = s.str();
			add_color_to_string(temp);
			WriteFile(cur_io_handle, temp.c_str(), (DWORD)(temp.size() * sizeof(wchar_t)), NULL, NULL);
		}

		if (clear) {
			clear_stream();
		}
	}

	inline void write_to_debug(bool clear = false, std::wstringstream& s = process_msg_stream) {
		OutputDebugString(s.str().c_str());
		if (clear) {
			clear_stream();
		}
	}

	inline void write_to_console(std::wstringstream& s = process_msg_stream) {
		// Add color
		auto temp_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (!temp_handle) {
			return;
		}
		std::wstring temp = s.str(); 
		add_color_to_string(temp); 
		WriteConsole(temp_handle, temp.c_str(), (DWORD)(temp.size()), NULL, NULL);
	}

	inline void print_intermediate_stream(std::wstringstream& s) {
		write_to_handle(false, io_handle, s);
		write_to_console(s);
	#ifdef _DEBUG
		write_to_debug(false, s);
	#endif // _DEBUG

	}

	template<typename T, typename... Targ>
	void unformated_log(T first, Targ... args) {
		create_msg(MSG_LEVEL::_NONE, first, args...);
	}

	template<typename T, typename... Targ>
	void log(T first, Targ... args) {
		create_msg(MSG_LEVEL::_INFO, first, args...);
	}

	template<typename T, typename... Targ>
	void warn(T first, Targ... args) {
		create_msg(MSG_LEVEL::_WARNING, first, args...);
	}

	template<typename T, typename... Targ>
	void error(T first, Targ... args) {
		create_msg(MSG_LEVEL::_ERROR, first, args...);
	}

	template<typename T, typename... Targ>
	void success(T first, Targ... args) {
		create_msg(MSG_LEVEL::_SUCCESS, first, args...);
	}
};

#endif // LOGGER_H
