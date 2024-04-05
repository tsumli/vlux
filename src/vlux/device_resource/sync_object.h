#ifndef SYNC_OBJECT_H
#define SYNC_OBJECT_H
#include "pch.h"

namespace vlux {

class SyncObject {
   public:
    SyncObject() = delete;
    SyncObject(const VkDevice device);
    ~SyncObject();

    // accessor
    VkSemaphore GetVkImageAvailableSemaphore() const { return image_available_semaphore_; }
    VkSemaphore GetvluxenderFinishedSemaphore() const { return render_finished_semaphore_; }
    VkFence GetVkInFlightFence() const { return in_flight_fence_; }

    // methods
    void WaitAndResetFences() const;

   private:
    const VkDevice device_;
    VkSemaphore image_available_semaphore_;
    VkSemaphore render_finished_semaphore_;
    VkFence in_flight_fence_;
};

}  // namespace vlux
#endif