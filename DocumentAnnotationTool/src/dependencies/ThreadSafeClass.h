#pragma once

#include <mutex>
#include <vector>
#include <deque>

template <typename T, typename M>
class RawSafeClass {
	T* obj = nullptr;
	std::unique_lock<M> lock;

public:
	RawSafeClass(M& m, T* p) : lock(m) {
		obj = p;
	}

	RawSafeClass(std::unique_lock<M>& l, T* p) {
		std::swap(l, lock);
		obj = p;
	}

	RawSafeClass(RawSafeClass&& a) noexcept {
		obj = a.obj;
		a.lock.swap(lock);
	}

	RawSafeClass& operator=(RawSafeClass&& t) noexcept {
		this->~RawSafeClass();
		new(this) RawSafeClass(std::move(t));
		return *this;
	}

	T* operator->()const { return obj; }
	T& operator*() const { return *obj; }

	~RawSafeClass() {
		// will not delete pointer since it is not owning it
		lock.unlock();
	}
};

template<typename T, typename M = std::recursive_mutex, typename C = RawSafeClass<T, M>>
struct ThreadSafeClass {
private:
	T m_item;
	M m_mutex;
public:
	ThreadSafeClass(T&& item) {
		m_item = std::move(item);
	}

	C get_item() {
		return C(m_mutex, &m_item);
	}

	//std::optional<C> try_get_item() {
	//	// TODO FIX IT
	//	std::unique_lock<M> lock(m_mutex, std::defer_lock);
	//	bool succ = lock.try_lock();
	//	if (!succ)
	//		return std::nullopt;
	//	return C(m_mutex, &m_item);
	//}

	ThreadSafeClass(const ThreadSafeClass& o) = delete;
	ThreadSafeClass(ThreadSafeClass&& o) = delete;
	ThreadSafeClass& operator=(const ThreadSafeClass& t) = delete;
	ThreadSafeClass& operator=(ThreadSafeClass&& t) = delete;


	~ThreadSafeClass() {
		// make sure mutex is not used
		get_item();
	}
};

