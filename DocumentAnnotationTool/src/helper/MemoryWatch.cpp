#include "include.h"

size_t bytesAllocated = 0;
size_t offset = 0;
HANDLE h_procheap = nullptr;

size_t MemoryWatcher::get_allocated_bytes() {
	return bytesAllocated - offset;
}

size_t MemoryWatcher::get_all_allocated_bytes() {
	return bytesAllocated;
}

void MemoryWatcher::set_calibration() {
	offset = bytesAllocated;
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __cdecl operator new(size_t _Size) {
	if (h_procheap == nullptr)
		h_procheap = GetProcessHeap();

	void* ptr = HeapAlloc(h_procheap, HEAP_ZERO_MEMORY, _Size);
	if (ptr == nullptr) {
		throw std::bad_alloc(); 
	}
	bytesAllocated += _Size;
	return ptr;
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __cdecl operator new[](size_t _Size) { 
	return ::operator new(_Size); 
}

void operator delete[](void* ptr) noexcept {
	::operator delete(ptr);
}

void operator delete(void* ptr) noexcept {
	if (h_procheap == nullptr)
		h_procheap = GetProcessHeap();

	bytesAllocated -= HeapSize(h_procheap, 0, ptr);
	HeapFree(h_procheap, 0, ptr);
}
