export module Docanto:Timer;

import <chrono>;
import <sstream>;

namespace Docanto {
    export class Timer {
    private:
        std::chrono::time_point<std::chrono::steady_clock> start_time;
    public:
        Timer();
        Timer(const Timer& other);
        Timer(Timer&& other) noexcept;
        Timer& operator=(Timer other);
        ~Timer();

        template<typename T>
        long long delta() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<T>(now - start_time).count();
        }

        long long delta_ns() const;
        long long delta_us() const;
        long long delta_ms() const;
        long long delta_s() const;
        long long delta_m() const;
        long long delta_h() const;

        std::basic_ostream<wchar_t>& get_string_representation(std::basic_ostream<wchar_t>& s) const;

        friend auto& operator<<(std::basic_ostream<wchar_t>& s, const Timer& p);
        friend void swap(Timer& first, Timer& second);
    };

    export auto& operator<<(std::basic_ostream<wchar_t>& s, const Docanto::Timer& p) {
        return p.get_string_representation(s);
    }
}
