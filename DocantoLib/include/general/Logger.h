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
		};

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

		template <StreamableTuple T>
		void do_msg(T msg) {
			size_t size = std::tuple_size<T>{};
			*_msg_buffer << "{";
			std::apply([&](auto&&... args) {
				size_t index = 0;
				((do_msg(args), *_msg_buffer << (++index < size ? ", " : "")), ...);
				}, msg);
			*_msg_buffer << "}";
		}

		template <StreamableContainer T>
		void do_msg(T arr) {
			size_t size = arr.size();
			*_msg_buffer << "[";
			for (const auto& i : arr) {
				do_msg(i);
				*_msg_buffer << (--size > 0 ? ", " : "");
			}
			*_msg_buffer << "]";
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
