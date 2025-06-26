#include "Common.h"

namespace Docanto {

	template<typename T, typename _mutex_type>
	class ThreadSafeObj;


	template<typename T, typename _mutex_type = std::recursive_mutex>
	class ThreadSafeWrapper {
	protected:
		T obj;
		_mutex_type mutex;

	public:
		ThreadSafeWrapper(T&& obj) : obj(std::move(obj)) {}

		ThreadSafeObj<T, _mutex_type> get() {
			return ThreadSafeObj<T, _mutex_type>(this);
		}

		std::optional<ThreadSafeObj<T, _mutex_type>> try_get() {
			std::unique_lock<_mutex_type> lock(mutex, std::defer_lock);

			if (lock.try_lock()) {
				return std::optional<ThreadSafeObj<T, _mutex_type>>(std::in_place, this, std::move(lock));
			}
			return std::nullopt;
		}

		ThreadSafeWrapper() = delete;
		ThreadSafeWrapper(const ThreadSafeWrapper&) = delete;
		ThreadSafeWrapper(ThreadSafeWrapper&&) noexcept = delete;
		ThreadSafeWrapper& operator=(const ThreadSafeWrapper&) = delete;
		ThreadSafeWrapper& operator=(ThreadSafeWrapper&&) noexcept = delete;


		template<typename T, typename _mutex_type>
		friend class ThreadSafeObj;
	};


	template<typename T, typename _mutex_type = std::recursive_mutex>
	class ThreadSafeObj {
		ThreadSafeWrapper<T, _mutex_type>* ref;
		std::unique_lock<_mutex_type> lock;

	protected:
		ThreadSafeObj(ThreadSafeWrapper<T, _mutex_type>* r) : ref(r), lock(ref->mutex) {}
	public:
		ThreadSafeObj(ThreadSafeWrapper<T, _mutex_type>* r, std::unique_lock<_mutex_type>&& l) : ref(r), lock(std::move(l)) {}
		~ThreadSafeObj() {}

		ThreadSafeObj(const ThreadSafeObj&) = delete;
		ThreadSafeObj& operator=(const ThreadSafeObj&) = delete;
		ThreadSafeObj(ThreadSafeObj&&) = delete;
		ThreadSafeObj& operator=(ThreadSafeObj&&) = delete;


		T* get() const {
			return &(ref->obj);
		}

		T* operator->()const { return &(ref->obj); }
		T& operator*() const { return (ref->obj); }


		template<typename T, typename _mutex_type>
		friend class ThreadSafeWrapper;
	};
}