#include "frame_timer.h"

namespace vlux {
FrameTimer::FrameTimer() { prev_update_ = frame_start_ = std::chrono::system_clock::now(); }

void FrameTimer::Update() {
    const auto cur = std::chrono::system_clock::now();
    elapsed_ = GetDurationSeconds(frame_start_, cur);
    const auto elapsed_prev_update = GetDurationSeconds(prev_update_, cur);
    if (elapsed_prev_update >= 1.0f) {
        fps_ = static_cast<float>(count_) / elapsed_prev_update;

        // Reset
        count_ = 0;
        prev_update_ = cur;
    }
    count_++;
    frame_start_ = cur;
}

float GetDurationSeconds(std::chrono::system_clock::time_point start,
                         std::chrono::system_clock::time_point end) {
    auto ret = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return ret / 1e6f;
};
}  // namespace vlux