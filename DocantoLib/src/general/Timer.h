#include "Common.h"

#ifndef _TIMER_H_
#define _TIMER_H_


namespace Docanto {

    class Timer_impl;

	class Timer {
    public:
        Timer();
        Timer(const Timer& other);
        Timer(Timer&& other) noexcept;
        Timer& operator=(Timer& other);
        Timer& operator=(Timer&& other) noexcept;
        ~Timer() = default;

        long long delta_ns() const;
        long long delta_us() const;
        long long delta_ms() const;
        long long delta_s() const;
        long long delta_m() const;
        long long delta_h() const;

        std::wostream& to_string(std::wostream& s) const;


    private:
        std::shared_ptr<Timer_impl> time;
	};
}

std::wostream& operator<<(std::wostream& os, const Docanto::Timer& timer);

#endif // _TIMER_H_