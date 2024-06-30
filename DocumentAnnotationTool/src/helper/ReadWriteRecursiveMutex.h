#pragma once

#include <thread>
#include <shared_mutex>
#include <tuple>
#include <condition_variable>
#include <map>
#include <optional>


struct ReadWriteThreadSafeClassReadMutexInfo {
	std::mutex m;
	std::map<std::thread::id, std::pair<size_t, std::shared_lock<std::shared_mutex>>> global_map;

	bool write_request = false;

	std::unique_lock<std::shared_mutex> unique_lock;
	std::condition_variable unique_shared_var;
	size_t unique_counter = 0;
	std::optional<std::thread::id> unique_id = std::nullopt;
};

template<typename T>
class ReadSafeClass {
	const T* obj;
	std::shared_ptr<ReadWriteThreadSafeClassReadMutexInfo> read_lock_info;
public:
	ReadSafeClass(const T* obj, std::shared_ptr<ReadWriteThreadSafeClassReadMutexInfo> l, std::shared_mutex& shared_m) : read_lock_info(l), obj(obj) {
		std::thread::id t_id = std::this_thread::get_id();

		// get acces to the info
		{
			std::unique_lock<std::mutex> lock(read_lock_info->m);

			// ___---___ Owns unique mutex ___---___
			if (read_lock_info->unique_id.has_value() and read_lock_info->unique_id == t_id) {
				// check if this thread has already a read lock
				if (read_lock_info->global_map.find(t_id) != read_lock_info->global_map.end()) {
					// case if the lock was already acquired just increase the counter
					read_lock_info->global_map.at(t_id).first++;
					return;
				}
				// we can defer lock the read since we know that there is a unique lock 
				read_lock_info->global_map.emplace(t_id, std::make_pair(1, std::shared_lock<std::shared_mutex>(shared_m, std::defer_lock)));
				return;
			}

			// ___---___ Not owned locked unique lock ___---___
			// deadlock avoidance since if a thread has the unique lock we have to unlock the map mutex
			// so they can deconstruct their lock
			if (read_lock_info->unique_id != std::nullopt or read_lock_info->write_request) {
				read_lock_info->unique_shared_var.wait(lock, [this] {
					return read_lock_info->unique_id == std::nullopt and read_lock_info->write_request == false;
					});
			}

			// ___---___ Owned shared lock ___---___
			// check if this thread already locked
			if (read_lock_info->global_map.find(t_id) != read_lock_info->global_map.end()) {
				// case if the lock was already acquired just increase the counter
				read_lock_info->global_map.at(t_id).first++;
			}
			// ___---___ No locks ___---___
			else {
				// case where the lock has not been acquired
				read_lock_info->global_map.emplace(t_id, std::make_pair(1, std::shared_lock<std::shared_mutex>(shared_m)));
			}
		}

	}

	ReadSafeClass(ReadSafeClass&& a) noexcept {
		obj = std::move(a.obj);
		read_lock_info = std::move(a.read_lock_info);
	}

	ReadSafeClass& operator=(ReadSafeClass&& t) noexcept {
		this->~ReadSafeClass();
		new(this) ReadSafeClass(std::move(t));
		return *this;
	}

	const T* get() const {
		return obj;
	}

	const T* operator->()const { return obj; }
	const T& operator*() const { return *obj; }

	~ReadSafeClass() {
		if (read_lock_info == nullptr)
			return;

		std::thread::id t_id = std::this_thread::get_id();
		// get acces to the info
		{
			std::unique_lock<std::mutex> lock(read_lock_info->m);
			// check if this thread already locked
			if (read_lock_info->global_map.find(t_id) == read_lock_info->global_map.end()) {
				// we should never be here!
				abort();
			}
			read_lock_info->global_map.at(t_id).first--;
			if (read_lock_info->global_map.at(t_id).first == 0) {
				// we can remove the entry from the map
				read_lock_info->global_map.erase(t_id);
				// this should also automatically remove the lock
			}
		}

		read_lock_info->unique_shared_var.notify_all();
		read_lock_info = nullptr;
	}
};

template<typename T>
class WriteSafeClass {
	T* obj;

	std::shared_ptr<ReadWriteThreadSafeClassReadMutexInfo> read_lock_info;
public:
	WriteSafeClass(T* obj, std::shared_ptr<ReadWriteThreadSafeClassReadMutexInfo> l, std::shared_mutex& shared_m) : read_lock_info(l), obj(obj) {
		std::thread::id t_id = std::this_thread::get_id();

		// get acces to the info
		{
			std::unique_lock<std::mutex> lock(read_lock_info->m);
			size_t shared_calls = 0;

			// ___---___ Owns unique lock ___---___
			if (read_lock_info->unique_id.has_value() and read_lock_info->unique_id.value() == t_id) {
				read_lock_info->unique_counter += 1;
				return;
			}

			// ___---___ Owns a shared lock ___---___
			// first check if the calling thread has a read lock to avoid deadlock
			if (read_lock_info->global_map.find(t_id) != read_lock_info->global_map.end()) {
				// if there is a write request we have to remove the read lock completly from our list
				shared_calls = read_lock_info->global_map.at(t_id).first;
				read_lock_info->global_map.erase(t_id);

				// after which we have to wait until our read lock is the last in the map and no unique lock is locked
				if (read_lock_info->global_map.empty() and read_lock_info->unique_id.has_value() == false) {
					read_lock_info->unique_lock = std::move(std::unique_lock<std::shared_mutex>(shared_m));
					read_lock_info->unique_id = t_id;
					read_lock_info->unique_counter = 1;

					// add the read lock back into the map so it can be locker later
					read_lock_info->global_map.emplace(t_id, std::make_pair(shared_calls, std::shared_lock<std::shared_mutex>(shared_m, std::defer_lock)));

					return;
				}
			}

			// ___---___ Shared or unique locks are locked  ___---___
			// we have to check if there are any read or write locks
			if (read_lock_info->global_map.empty() == false or read_lock_info->unique_id.has_value()) {
				// we need to wait until all read locks are freed
				read_lock_info->write_request = true;
				read_lock_info->unique_shared_var.wait(lock, [this] {
					return read_lock_info->global_map.empty() and read_lock_info->unique_id == std::nullopt;
					});
				read_lock_info->write_request = false;
			}

			if (shared_calls != 0) {
				// add the read lock back into the map so it can be locker later
				read_lock_info->global_map.emplace(t_id, std::make_pair(shared_calls, std::shared_lock<std::shared_mutex>(shared_m, std::defer_lock)));
			}

			// we shouldn't be here
			if (read_lock_info->unique_id.has_value()) {
				abort();
			}

			// ___---___ All locks are unlocked  ___---___
			read_lock_info->unique_lock = std::move(std::unique_lock<std::shared_mutex>(shared_m));
			read_lock_info->unique_id = t_id;
			read_lock_info->unique_counter = 1;
		}
	}

	WriteSafeClass(WriteSafeClass&& a) noexcept {
		obj = std::move(a.obj);
		read_lock_info = std::move(a.read_lock_info);
	}

	WriteSafeClass& operator=(WriteSafeClass&& t) noexcept {
		this->~WriteSafeClass();
		new(this) WriteSafeClass(std::move(t));
		return *this;
	}

	T* get() const {
		return obj;
	}

	T* operator->()const { return obj; }
	T& operator*() const { return *obj; }

	~WriteSafeClass() {
		if (read_lock_info == nullptr)
			return;

		std::thread::id t_id = std::this_thread::get_id();
		// get acces to the info
		{
			std::unique_lock<std::mutex> lock(read_lock_info->m);

			if (read_lock_info->unique_id.has_value() == false or read_lock_info->unique_id.value() != t_id) {
				abort();
			}

			read_lock_info->unique_counter -= 1;

			if (read_lock_info->unique_counter == 0) {
				// release lock
				read_lock_info->unique_id = std::nullopt;
				read_lock_info->unique_lock.unlock();

				// lock any possible read locks that have been acquired by the same thread
				if (read_lock_info->global_map.find(t_id) != read_lock_info->global_map.end()) {
					read_lock_info->global_map.at(t_id).second.lock();
				}

				// notify all possible read threads
				lock.unlock();
				read_lock_info->unique_shared_var.notify_all();

				read_lock_info = nullptr;
				return;
			}

		}

		read_lock_info = nullptr;
	}
};

template<typename T>
class ReadWriteThreadSafeClass {
	T obj;
	std::shared_mutex m;

	std::shared_ptr<ReadWriteThreadSafeClassReadMutexInfo> info =
		std::shared_ptr<ReadWriteThreadSafeClassReadMutexInfo>(new ReadWriteThreadSafeClassReadMutexInfo());
public:
	ReadWriteThreadSafeClass(T&& obj) {
		obj = std::move(obj);
	}

	WriteSafeClass<T> get_write() {
		return WriteSafeClass<T>(&obj, info, m);
	}

	ReadSafeClass<T> get_read() {
		return ReadSafeClass<T>(&obj, info, m);
	}

	~ReadWriteThreadSafeClass() {
		get_read();
	}
};