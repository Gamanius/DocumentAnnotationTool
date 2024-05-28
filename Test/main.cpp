#include <thread>
#include <iostream>
#include <mutex>
#include <map>
#include <memory>

class ThreadSafeWrapper;
class ThreadSafe {
	std::mutex mutex;
public:
	friend ThreadSafeWrapper;
};

class ThreadSafeWrapper {
	std::shared_ptr<ThreadSafe> obj;
	std::unique_lock<std::mutex> lock;
public:
	ThreadSafeWrapper() {}

	ThreadSafeWrapper(std::shared_ptr<ThreadSafe> p) : lock(p->mutex) {
		std::cout << "Created class" << std::endl;
		obj = std::shared_ptr<ThreadSafe>(p);
	}

	ThreadSafeWrapper(ThreadSafeWrapper&& a) noexcept {
		obj = std::move(a.obj);
		a.lock.swap(lock);
	}

	ThreadSafeWrapper& operator=(ThreadSafeWrapper&& t) noexcept {
		this->~ThreadSafeWrapper();
		new(this) ThreadSafeWrapper(std::move(t));
		return *this;
	}

	ThreadSafeWrapper(const ThreadSafeWrapper& a) = delete;
	ThreadSafeWrapper& operator=(const ThreadSafeWrapper& a) = delete;


	ThreadSafe* operator->()const { return obj.get(); }
	ThreadSafe& operator*() const { return *obj; }
};

template <typename T>
class ThreadSafeClass {
	std::shared_ptr<T> obj;
	std::unique_lock<std::mutex> lock;

public:
	ThreadSafeClass(std::pair<std::mutex, std::shared_ptr<T>>& p) : lock(p.first) {
		obj = std::shared_ptr<T>(p.second);
	}

	ThreadSafeClass(std::mutex& m, std::shared_ptr<T> p) : lock(m) {
		std::cout << "Created class" << std::endl;
		obj = std::shared_ptr<T>(p);
	}

	ThreadSafeClass(ThreadSafeClass&& a) noexcept {
		obj = std::move(a.obj);
		a.lock.swap(lock);
	}

	ThreadSafeClass& operator=(ThreadSafeClass&& t) noexcept {
		this->~ThreadSafeClass();
		new(this) ThreadSafeClass(std::move(t));
		return *this;
	}

	T* operator->()const { return obj.get(); }
	T& operator*() const { return *obj; }

	~ThreadSafeClass() {
		lock.unlock();
		std::cout << "Killed Threadsafe class" << std::endl;
	}
};


template <typename T>
class RawThreadSafeClass {
	T* obj = nullptr;
	std::unique_lock<std::recursive_mutex> lock;
public:
	RawThreadSafeClass(std::recursive_mutex& m, T* p) {
		obj = p;
		lock = std::unique_lock<std::recursive_mutex>(m);
		std::cout << "Created class" << std::endl;
	}

	RawThreadSafeClass(RawThreadSafeClass&& a) noexcept {
		obj = a.obj;
		a.lock.swap(lock);
	}

	RawThreadSafeClass& operator=(RawThreadSafeClass&& t) noexcept {
		this->~RawThreadSafeClass();
		new(this) RawThreadSafeClass(std::move(t));
		return *this;
	}

	T* operator->()const { return obj; }
	T& operator*() const { return *obj; }

	~RawThreadSafeClass() {
		std::cout << "Deleted Threadsafe class" << std::endl;
	}
};


class CoolClass {
	int a = 0;
public:

	CoolClass() = default;

	int get_a() {
		std::cout << "Got a: " << a << std::endl;
		return a;
	}

	void change(int a) {
		std::cout << "Changed a to " << a << std::endl;
		this->a = a;
	}

	~CoolClass() {
		a = 1000;
		std::cout << "Delted Cool Class" << std::endl;
	}
};

class CoolClass2 {
	int a = 0;
	std::recursive_mutex m;
public:
	CoolClass2() = default;

	RawThreadSafeClass<int> get_a() {
		return RawThreadSafeClass<int>(m, &a);
	}
};

std::shared_ptr<CoolClass> test_class;
CoolClass2 test_class2;


void test() {
	auto a = test_class2.get_a();
	a = test_class2.get_a();
	a = test_class2.get_a();
	a = test_class2.get_a();
	a = test_class2.get_a();
	a = test_class2.get_a();
	a = test_class2.get_a();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	*a = *a + 1;
}


int main() {
	std::thread t1(test);
	std::thread t2(test);

	t1.join();
	t2.join();

	auto a = test_class2.get_a();
	std::cout << "A is now " << *a << std::endl;

	return 0;
}