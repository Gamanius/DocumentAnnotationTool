#include "Timer.h"

#include <chrono>

using namespace Docanto;
using namespace std::chrono;

class Docanto::Timer_impl {
public:
	high_resolution_clock::time_point start;

	Timer_impl() {
		start = high_resolution_clock::now();
	}

	Timer_impl(const Timer_impl& other) = delete;
	Timer_impl& operator=(Timer_impl other) = delete;

	template<typename T>
	long long delta() const {
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<T>(now - start).count();
	}
};

Docanto::Timer::Timer() {
	time = std::shared_ptr<Timer_impl>(new Timer_impl());
}

Docanto::Timer::Timer(const Timer& other) {
	this->time = other.time;
}

Docanto::Timer::Timer(Timer&& other) noexcept : time(other.time) {
	other.time = nullptr;
}

Docanto::Timer& Docanto::Timer::operator=(Timer& other) {
	this->time = other.time;

	return *this;
}

Docanto::Timer& Docanto::Timer::operator=(Timer&& other) noexcept {
	std::swap(this->time, other.time);

	return *this;
}

long long Timer::delta_ns() const {
	return time->delta<std::chrono::nanoseconds>();
}

long long Timer::delta_us() const {
	return time->delta<std::chrono::microseconds>();
}

long long Timer::delta_ms() const {
	return time->delta<std::chrono::milliseconds>();
}

long long Timer::delta_s() const {
	return time->delta<std::chrono::seconds>();
}

long long Timer::delta_m() const {
	return time->delta<std::chrono::minutes>();
}

long long Timer::delta_h() const {
	return time->delta<std::chrono::hours>();
}

std::wostream& Docanto::Timer::to_string(std::wostream& sstream) const {
	auto us = delta_us();
	if (us < 1000) {
		return sstream << us << L"µs";
	}

	auto ms = delta_ms();
	if (ms < 1000) {
		return sstream << ms << L"ms";
	}

	auto s = ms / 1000;
	ms = ms % 1000;
	if (s < 60) {
		return sstream << s << L"s " << ms << L"ms";
	}

	auto m = s / 60;
	s = s % 60;
	if (m < 60) {
		return sstream << m << L"m " << s << L"s " << ms << L"ms";
	}

	auto h = m / 60;
	m = m % 60;
	return sstream << h << L"h " << m << L"m " << s << L"s " << ms << L"ms";
}

std::wostream& operator<<(std::wostream& os, const Docanto::Timer& timer) {
	timer.to_string(os);
	return os;
}
