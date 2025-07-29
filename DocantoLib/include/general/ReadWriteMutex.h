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

		struct QueueItem {
			std::thread::id id;
			ACCESS_TYPE requested_access = ACCESS_TYPE::NO_ACCESS;
		};

		std::mutex wrapper_mutex;
		std::condition_variable wrapper_conditional;

		std::map<std::thread::id, ThreadInfo> thread_list;
		std::queue<QueueItem> wait_queue;
	

		void add_read(std::thread::id id) {
			if (thread_list.find(id) == thread_list.end()) {
				thread_list[id] = { 0, 0, ACCESS_TYPE::READ_ACCESS };
			}

			thread_list[id].read_locks++;
		}

		void add_write(std::thread::id id) {
			if (thread_list.find(id) == thread_list.end()) {
				thread_list[id] = { 0, 0, ACCESS_TYPE::WRITE_ACCESS };
			}

			thread_list[id].write_locks++;
		}

		void remove_read(std::thread::id id) {
			if (thread_list.find(id) == thread_list.end()) { return; }

			thread_list[id].read_locks--;

			// if there is no lock remaining well remove it
			if (thread_list[id].write_locks == 0 and thread_list[id].read_locks == 0) {
				thread_list.erase(id);
				return;
			}
		}

		void remove_write(std::thread::id id) {
			if (thread_list.find(id) == thread_list.end()) { return; }

			thread_list[id].write_locks--;

			// if there are no lock remaining we can remove it
			if (thread_list[id].write_locks == 0 and thread_list[id].read_locks == 0) {
				thread_list.erase(id);
				return;
			}

			// if there is at least one read lock remaining we give it read access back
			if (thread_list[id].write_locks == 0 and thread_list[id].read_locks != 0) {
				thread_list[id].access = ACCESS_TYPE::READ_ACCESS;
			}
		}

		bool can_write(std::thread::id id) {
			for (auto& [list_id, item] : thread_list) {
				// if there is any thread which hold a lock of any kind
				// we cannot write
				if (list_id != id) {
					return false;
				}
			}
			return true;
		}

		bool can_read(std::thread::id id) {
			for (auto& [list_id, item] : thread_list) {
				// we only cant read if there is another thread which has write access
				if (list_id != id and item.access == ACCESS_TYPE::WRITE_ACCESS) {
					return false;
				}
			}
			return true;
		}
	public:
		ReadWriteThreadSafeMutex(T&& item) : item(std::move(item)) {}
		ReadWriteThreadSafeMutex() : item(T()) {}

		ReadWriteThreadSafeMutex(const ReadWriteThreadSafeMutex& other) = delete;
		ReadWriteThreadSafeMutex& operator=(const ReadWriteThreadSafeMutex& other) = delete;

		ReadWriteThreadSafeMutex(ReadWriteThreadSafeMutex&& other) noexcept = delete;
		ReadWriteThreadSafeMutex& operator=(ReadWriteThreadSafeMutex&& other) noexcept = delete;

		WriteWrapper<T> get_write() {
			return WriteWrapper<T>(&item, this);
		}

		ReadWrapper<T> get_read() {
			return ReadWrapper<T>(&item, this);
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
			auto local_id = std::this_thread::get_id();

			std::unique_lock<std::mutex> lock(wrapper->wrapper_mutex);
			// check the position of the queue
			if (wrapper->wait_queue.size() == 0 and wrapper->can_read(local_id)) {
				// now we need to check if there is another thread which is locking
				wrapper->add_read(local_id);
				wrapper->wrapper_conditional.notify_all();
				return;
			}

			// check if we already have a read lock to avoid deadlock
			if (wrapper->thread_list.find(local_id) != wrapper->thread_list.end()) {
				wrapper->add_read(local_id);
				wrapper->wrapper_conditional.notify_all();
				return;
			}

			// else we need to wait until its our turn
			wrapper->wait_queue.push({ local_id, wrapper->ACCESS_TYPE::READ_ACCESS });
			wrapper->wrapper_conditional.wait(lock, [&]() -> bool {
				const auto& it = wrapper->wait_queue.front();
				return it.id == local_id and it.requested_access == wrapper->ACCESS_TYPE::READ_ACCESS and wrapper->can_read(local_id);
			});

			wrapper->wait_queue.pop();
			wrapper->add_read(local_id);

			wrapper->wrapper_conditional.notify_all();
		}

		~ReadWrapper() {
			if (wrapper == nullptr) {
				return;
			}

			std::unique_lock<std::mutex> lock(wrapper->wrapper_mutex);
			auto local_id = std::this_thread::get_id();

			wrapper->remove_read(local_id);
			wrapper->wrapper_conditional.notify_all();
		}

		ReadWrapper(const ReadWrapper& other) = delete;
		ReadWrapper& operator=(const ReadWrapper& other) = delete;

		ReadWrapper(ReadWrapper&& other) noexcept {
			this->wrapper->remove_read(std::this_thread::get_id());
			this->wrapper = other.wrapper;
			other.wrapper = nullptr;

			this->obj = other.obj;
			other.obj = nullptr;
		}

		ReadWrapper& operator=(ReadWrapper&& other) noexcept {
			if (this != &other) {
				this->wrapper->remove_read(std::this_thread::get_id());
				this->wrapper = other.wrapper;
				other.wrapper = nullptr;

				this->obj = other.obj;
				other.obj = nullptr;
			}
			return *this;
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
			auto local_id = std::this_thread::get_id();

			std::unique_lock<std::mutex> lock(wrapper->wrapper_mutex);
			// check the position of the queue
			if (wrapper->wait_queue.size() == 0 and wrapper->can_write(local_id)) {
				// now we need to check if there is another thread which is locking
				wrapper->add_write(local_id);
				wrapper->wrapper_conditional.notify_all();
				return;
			}

			// check if we already have a write lock to avoid deadlock
			if (wrapper->thread_list.find(local_id) != wrapper->thread_list.end() and 
				wrapper->thread_list[local_id].access == wrapper->ACCESS_TYPE::WRITE_ACCESS) {
				wrapper->add_write(local_id);
				wrapper->wrapper_conditional.notify_all();
				return;
			}

			// else we need to wait until its our turn
			wrapper->wait_queue.push({ local_id, wrapper->ACCESS_TYPE::WRITE_ACCESS });
			wrapper->wrapper_conditional.wait(lock, [&]() -> bool {
				const auto& it = wrapper->wait_queue.front();
				return it.id == local_id and it.requested_access == wrapper->ACCESS_TYPE::WRITE_ACCESS and wrapper->can_write(local_id);
				});

			wrapper->wait_queue.pop();
			wrapper->add_write(local_id);

			wrapper->wrapper_conditional.notify_all();
		}


		~WriteWrapper() {
			if (wrapper == nullptr) {
				return;
			}

			std::unique_lock<std::mutex> lock(wrapper->wrapper_mutex);
			auto local_id = std::this_thread::get_id();
			
			wrapper->remove_write(local_id);

			wrapper->wrapper_conditional.notify_all();
		}

		WriteWrapper(const WriteWrapper& other) = delete;
		WriteWrapper& operator=(const WriteWrapper& other) = delete;

		WriteWrapper(WriteWrapper&& other) noexcept {
			this->wrapper->remove_write(std::this_thread::get_id());
			this->wrapper = other.wrapper;
			other.wrapper = nullptr;

			this->obj = other.obj;
			other.obj = nullptr;
		}

		WriteWrapper& operator=(WriteWrapper&& other) noexcept {
			if (this != &other) {
				this->wrapper->remove_write(std::this_thread::get_id());
				this->wrapper = other.wrapper;
				other.wrapper = nullptr;

				this->obj = other.obj;
				other.obj = nullptr;
			}
			return *this;
		}



		T* get() const {
			return obj;
		}

		T* operator->()const { return obj; }
		T& operator*() const { return *obj; }
	};
}

template <typename T>
using ThreadSafeVector = Docanto::ReadWriteThreadSafeMutex<std::vector<T>>;

template <typename T>
using ThreadSafeDeque = Docanto::ReadWriteThreadSafeMutex<std::deque<T>>;

template <typename T, typename U>
using ThreadSafeMap = Docanto::ReadWriteThreadSafeMutex<std::map<T, U>>;

template<typename T>
std::wostream& operator<<(std::wostream& os, const Docanto::ReadWrapper<T>& obj) {
	return os << *obj;
}

template<typename T>
std::wostream& operator<<(std::wostream& os, const Docanto::WriteWrapper<T>& obj) {
	return os << *obj;
}

#endif // !_READWRITEMUTEX_H_