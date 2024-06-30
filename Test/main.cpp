#include <thread>
#include <iostream>
#include <shared_mutex>
#include <tuple>
#include <condition_variable>
#include <map>
#include <optional>
#include <syncstream>


struct ThreadSafeClassReadMutexInfo {
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
	std::shared_ptr<ThreadSafeClassReadMutexInfo> read_lock_info;
public:
	ReadSafeClass(const T* obj, std::shared_ptr<ThreadSafeClassReadMutexInfo> l, std::shared_mutex& shared_m) : read_lock_info(l), obj(obj) {
		std::thread::id t_id = std::this_thread::get_id();
		// get acces to the info
		{
			std::unique_lock<std::mutex> lock(read_lock_info->m);

			// check if unique lock is already acquired
			if (read_lock_info->unique_id.has_value() and read_lock_info->unique_id == t_id) {
				// check if this thread has already a read lock
				if (read_lock_info->global_map.find(t_id) != read_lock_info->global_map.end()) { 
					// case if the lock was already acquired just increase the counter
					read_lock_info->global_map.at(t_id).first++;
					std::osyncstream(std::cout) << "Reaquired read " << t_id << "\n";
					return;
				} 
				// we can defer lock the read since we know that there is a unique lock 
				read_lock_info->global_map.emplace(t_id, std::make_pair(1, std::shared_lock<std::shared_mutex>(shared_m, std::defer_lock))); 
				std::osyncstream(std::cout) << "Defer read " << t_id << "\n";
				return;
			}

			// deadlock avoidance since if a thread has the unique lock we have to unlock the map mutex
			// so they can deconstruct their lock
			if (read_lock_info->unique_id != std::nullopt or read_lock_info->write_request) {
				read_lock_info->unique_shared_var.wait(lock, [this] {
					return read_lock_info->unique_id == std::nullopt and read_lock_info->write_request == false;
				});
			}

			// check if this thread already locked
			if (read_lock_info->global_map.find(t_id) != read_lock_info->global_map.end()) {
				// case if the lock was already acquired just increase the counter
				read_lock_info->global_map.at(t_id).first++;
				std::osyncstream(std::cout) << "Reaquired read " << t_id << "\n";
			}
			else {
				// case where the lock has not been acquired
				read_lock_info->global_map.emplace(t_id, std::make_pair(1, std::shared_lock<std::shared_mutex>(shared_m)));
				std::osyncstream(std::cout)  << "Aquired read " << t_id << "\n";
			}
		}

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
				std::osyncstream(std::cout) << "Delete read " << t_id << "\n";
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

	std::shared_ptr<ThreadSafeClassReadMutexInfo> read_lock_info;
public:
	WriteSafeClass(T* obj, std::shared_ptr<ThreadSafeClassReadMutexInfo> l, std::shared_mutex& shared_m) : read_lock_info(l), obj(obj) {
		std::thread::id t_id = std::this_thread::get_id(); 
		// get acces to the info
		{
			std::unique_lock<std::mutex> lock(read_lock_info->m);
			if (read_lock_info->unique_id.has_value() and read_lock_info->unique_id.value() == t_id) {
				read_lock_info->unique_counter += 1;	
				std::osyncstream(std::cout)  << "Reacquired write " << t_id << "\n";
				return;
			}
		}

		// wait for access
		{
			std::unique_lock<std::mutex> lock(read_lock_info->m);
			size_t shared_calls = 0;
			// first check if the calling thread has a read lock to avoid deadlock
			if (read_lock_info->global_map.find(t_id) != read_lock_info->global_map.end()) { 
				// if there is a write request we have to remove the read lock completly from our list
				shared_calls = read_lock_info->global_map.at(t_id).first;
				read_lock_info->global_map.erase(t_id);  
				std::osyncstream(std::cout) << "Unlock read " << t_id << "\n";

				// after which we have to wait until our read lock is the last in the map and no unique lock is locked
				if (read_lock_info->global_map.empty() and read_lock_info->unique_id.has_value() == false) { 
					read_lock_info->unique_lock = std::move(std::unique_lock<std::shared_mutex>(shared_m)); 
					read_lock_info->unique_id = t_id; 
					read_lock_info->unique_counter = 1; 

					// add the read lock back into the map so it can be locker later
					read_lock_info->global_map.emplace(t_id, std::make_pair(shared_calls, std::shared_lock<std::shared_mutex>(shared_m, std::defer_lock)));

					std::osyncstream(std::cout) << "Acquired write " << t_id << "\n";
					return;
				}
			}

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

			read_lock_info->unique_lock = std::move(std::unique_lock<std::shared_mutex>(shared_m)); 
			read_lock_info->unique_id = t_id;
			read_lock_info->unique_counter = 1; 
		}
		std::osyncstream(std::cout)  << "Acquired write " << t_id << "\n";
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

					std::osyncstream(std::cout) << "Locked read " << t_id << "\n";
				}

				// notify all possible read threads
				lock.unlock();
				std::osyncstream(std::cout)  << "Delete write " << t_id << "\n";
				read_lock_info->unique_shared_var.notify_all(); 

				read_lock_info = nullptr;
				return;
			}
			
		}

		read_lock_info = nullptr;
		std::osyncstream(std::cout)  << "Decremented write " << t_id << "\n";
	}
};

template<typename T>
class ThreadSafeClass {
	T obj;
	std::shared_mutex m;

	std::shared_ptr<ThreadSafeClassReadMutexInfo> info = 
		std::shared_ptr<ThreadSafeClassReadMutexInfo>(new ThreadSafeClassReadMutexInfo());
public:
	ThreadSafeClass(T&& obj) : obj(obj) {}

	WriteSafeClass<T> get_write() {
		return WriteSafeClass<T>(&obj, info, m);
	}

	ReadSafeClass<T> get_read() {
		return ReadSafeClass<T>(&obj, info, m);
	}
};

ThreadSafeClass<int> thread_safe_int(1);

void do_write() {
	auto d = thread_safe_int.get_write();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	*d += 2;
	auto c = thread_safe_int.get_read();
	d.~WriteSafeClass();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void do_read() {
	auto d = thread_safe_int.get_read();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::osyncstream(std::cout)  << *d << "\n";

	auto d1 = thread_safe_int.get_write();
	d.~ReadSafeClass();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {
	auto t1 = std::thread(do_write);
	auto t2 = std::thread(do_read);
	auto t3 = std::thread(do_write);
	auto t4 = std::thread(do_read);
	auto t5 = std::thread(do_read);
	auto t6 = std::thread(do_read);
	auto t7 = std::thread(do_write);
	
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
	auto d = thread_safe_int.get_read();

	auto d1 = thread_safe_int.get_write();


	return 0;
}