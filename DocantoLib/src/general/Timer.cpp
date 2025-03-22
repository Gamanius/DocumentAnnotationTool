module Docanto;
import :Timer;

using namespace Docanto;

Timer::Timer() {
	start_time = std::chrono::high_resolution_clock::now();
}

Timer::Timer(const Timer& other) : start_time(other.start_time) {}

Timer::Timer(Timer&& other) noexcept : Timer() {
	swap(*this, other);
}

Timer& Timer::operator=(Timer other) {
	swap(*this, other);
	return *this;
}

Timer::~Timer() {}

long long Timer::delta_ns() const {
	return delta<std::chrono::nanoseconds>();
}

long long Timer::delta_us() const {
	return delta<std::chrono::microseconds>();
}

long long Timer::delta_ms() const {
	return delta<std::chrono::milliseconds>();
}

long long Timer::delta_s() const {
	return delta<std::chrono::seconds>();
}

long long Timer::delta_m() const {
	return delta<std::chrono::minutes>();
}

long long Timer::delta_h() const {
	return delta<std::chrono::hours>();
}

std::basic_ostream<wchar_t>& Timer::get_string_representation(std::basic_ostream<wchar_t>& sstream) const {
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

namespace Docanto {
	void swap(Docanto::Timer& first, Docanto::Timer& second) {
		using std::swap;
		swap(first.start_time, second.start_time);
	}
}

