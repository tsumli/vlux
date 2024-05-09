#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#include <chrono>

namespace vlux {
class Timer {
   public:
    Timer() { Reset(); }

    void Reset() { start_time_ = std::chrono::high_resolution_clock::now(); }

    float ElapsedMilliseconds() const {
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        return duration.count() * 1e-3;
    }

   private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
};
}  // namespace vlux

#endif