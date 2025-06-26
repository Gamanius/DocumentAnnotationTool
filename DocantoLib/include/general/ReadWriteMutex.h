#include "Common.h"

#ifndef _READWRITEMUTEX_H_
#define _READWRITEMUTEX_H_

namespace Docanto {
	template <typename T>
	class ReadWrapper;

	template <typename T>
	class WriteWrapper;

	template <typename T>
	class ReadWriteThreadSafeMutex {
		T item;
	protected:
		enum class ACCESS_TYPE {
			READ_ACCESS,
			WRITE_ACCESS,
			NO_ACCESS,
		};

		struct ThreadInfo {
			size_t read_locks = 0;
			size_t write_locks = 0;
			ACCESS_TYPE access = ACCESS_TYPE::NO_ACCESS;
		};

		std::mutex thread_list_mutex;
		std::map<std::thread::id, ThreadInfo> thread_list;
		std::condition_variable thread_list_conditional;
	public:
		ReadWriteThreadSafeMutex(T&& item) : item(std::move(item)) {}

		ReadWriteThreadSafeMutex() = delete;
		ReadWriteThreadSafeMutex(const ReadWriteThreadSafeMutex& other) = delete;
		ReadWriteThreadSafeMutex& operator=(const ReadWriteThreadSafeMutex& other) = delete;

		ReadWriteThreadSafeMutex(ReadWriteThreadSafeMutex&& other) noexcept = delete;
		ReadWriteThreadSafeMutex& operator=(ReadWriteThreadSafeMutex&& other) noexcept = delete;

		WriteWrapper<T> get_write() {
			return std::move(WriteWrapper<T>(&item, this));
		}

		ReadWrapper<T> get_read() {
			return std::move(ReadWrapper<T>(&item, this));
		}

		friend class ReadWrapper<T>;
		friend class WriteWrapper<T>;
	};

	template <typename T>
	class ReadWrapper {
		const T* obj;
		ReadWriteThreadSafeMutex<T>* wrapper;
	public:
		ReadWrapper(const T* obj, ReadWriteThreadSafeMutex<T>* r) : wrapper(r), obj(obj) {
			std::unique_lock<std::mutex> lock(wrapper->thread_list_mutex);
			auto local_id = std::this_thread::get_id();

			// First try to find the thread in the map. If its already in there it has READ access guarantee
			if (wrapper->thread_list.find(local_id) != wrapper->thread_list.end()) {
				wrapper->thread_list[local_id].read_locks += 1;
				return;
			}

			// if this is not the case we have to check if there is any other thread who has write access
			auto check_access = [&]() -> bool {
				bool access = true;
				for (auto& [key, val] : wrapper->thread_list) {
					if (val.access == wrapper->ACCESS_TYPE::WRITE_ACCESS && key != local_id) {
						access = false;
					}
				}
				return access;
			};

			// first check if there are other threads that want to write
			wrapper->thread_list_conditional.wait(lock, check_access);

			std::cout << local_id << " has read access" << std::endl;
			// if everything is clear we can add our thread to the list of threads 
			wrapper->thread_list.insert({ local_id, {1, 0, wrapper->ACCESS_TYPE::READ_ACCESS} });
		}
		ReadWrapper(const ReadWrapper& other) = delete;
		ReadWrapper& operator=(const ReadWrapper& other) = delete;

		ReadWrapper(ReadWrapper&& other) noexcept : obj(other.obj), wrapper(other.wrapper) {
			other.wrapper = nullptr;
			other.obj = nullptr;
		}

		ReadWrapper& operator=(ReadWrapper&& other) noexcept {
			if (this != other) {
				this->wrapper = other.wrapper;
				other.wrapper = nullptr;

				this->obj = other.obj;
				other.obj = nullptr;
			}
			return *this;
		}



		~ReadWrapper() {
			// if wrapper is null then this class can be thrown away
			if (wrapper == nullptr) {
				return;
			}

			std::unique_lock<std::mutex> lock(wrapper->thread_list_mutex);
			auto local_id = std::this_thread::get_id();

			// first decrease the read locks 
			--wrapper->thread_list[local_id].read_locks;

			// if both read locks and write locks are 0 then we can remove this thread from the watch list
			if (wrapper->thread_list[local_id].read_locks == 0 and wrapper->thread_list[local_id].write_locks == 0) {
				wrapper->thread_list.erase(local_id);
				std::cout << local_id << " has removed access" << std::endl;
			}

			wrapper->thread_list_conditional.notify_all();
		}

		const T* get() const {
			return obj;
		}

		const T* operator->()const { return obj; }
		const T& operator*() const { return *obj; }
	};

	template <typename T>
	class WriteWrapper {
		T* obj;
		ReadWriteThreadSafeMutex<T>* wrapper;
	public:
		WriteWrapper(T* obj, ReadWriteThreadSafeMutex<T>* r) : wrapper(r), obj(obj) {
			std::unique_lock<std::mutex> lock(wrapper->thread_list_mutex);
			auto local_id = std::this_thread::get_id();

			// if we are in the list we may need to upgrade our permission
			if (wrapper->thread_list.find(local_id) != wrapper->thread_list.end()
				and wrapper->thread_list[local_id].access == wrapper->ACCESS_TYPE::WRITE_ACCESS) {

					wrapper->thread_list[local_id].write_locks += 1;
					return;
			}

			// since we dont already have write access we have to wait until every thread givevs back the object
			// this is only the case when the map is empty or has one entry containing this thread ID
			// check if we can have the access to the variable
			auto check_access = [&]() -> bool {
				bool access = wrapper->thread_list.empty() or 
					(wrapper->thread_list.find(local_id) != wrapper->thread_list.end() and wrapper->thread_list.size() == 1);
				return access;
				};

			wrapper->thread_list_conditional.wait(lock, check_access);


			std::cout << local_id << " has write access" << std::endl;

			// add to the map
			if (wrapper->thread_list.find(local_id) != wrapper->thread_list.end()) {
				wrapper->thread_list[local_id].write_locks += 1;
				wrapper->thread_list[local_id].access = wrapper->ACCESS_TYPE::WRITE_ACCESS;
			}
			else {
				wrapper->thread_list.insert({ local_id, {0, 1, wrapper->ACCESS_TYPE::WRITE_ACCESS} });
			}
		}

		WriteWrapper(const WriteWrapper& other) = delete;
		WriteWrapper& operator=(const WriteWrapper& other) = delete;

		WriteWrapper(WriteWrapper&& other) noexcept : obj(other.obj), wrapper(other.wrapper) {
			other.wrapper = nullptr;
			other.obj = nullptr;
		}

		WriteWrapper& operator=(WriteWrapper&& other) noexcept {
			if (this != other) {
				this->wrapper = other.wrapper;
				other.wrapper = nullptr;

				this->obj = other.obj;
				other.obj = nullptr;
			}
			return *this;
		}


		~WriteWrapper() {
			// if wrapper is null then this class can be thrown away
			if (wrapper == nullptr) {
				return;
			}

			std::unique_lock<std::mutex> lock(wrapper->thread_list_mutex);
			auto local_id = std::this_thread::get_id();

			--wrapper->thread_list[local_id].write_locks;
			// downgrade the access level
			if (wrapper->thread_list[local_id].write_locks == 0) {
				wrapper->thread_list[local_id].access = wrapper->ACCESS_TYPE::READ_ACCESS;

				std::cout << local_id << " has removed write access" << std::endl;
			}

			// if both read locks and write locks are 0 then we can remove this thread from the watch list
			if (wrapper->thread_list[local_id].read_locks == 0 and wrapper->thread_list[local_id].write_locks == 0) {
				wrapper->thread_list.erase(local_id);
				std::cout << local_id << " has removed access" << std::endl;
			}

			wrapper->thread_list_conditional.notify_all();

		}

		T* get() const {
			return obj;
		}

		T* operator->()const { return obj; }
		T& operator*() const { return *obj; }
	};

}


#endif // !_READWRITEMUTEX_H_