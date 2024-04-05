#ifndef FRAME_TIMER_H
#define FRAME_TIMER_H
#include "pch.h"

namespace vlux {
class FrameTimer {
   public:
    FrameTimer();
    FrameTimer(const FrameTimer&) = delete;
    FrameTimer& operator=(const FrameTimer&) = delete;
    FrameTimer(const FrameTimer&&) = delete;
    FrameTimer&& operator=(const FrameTimer&&) = delete;

    void Update();
    float GetFPS() const { return fps_; }
    float GetElapsedSeconds() const { return elapsed_; };

   private:
    std::chrono::system_clock::time_point prev_update_, frame_start_;
    uint32_t count_{0};
    float fps_{0.0f};
    float elapsed_{1.0f};  // to prevent zero-division
};
float GetDurationSeconds(std::chrono::system_clock::time_point start,
                         std::chrono::system_clock::time_point end);
}  // namespace vlux
#endif