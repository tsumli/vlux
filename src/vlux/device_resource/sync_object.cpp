#include "sync_object.h"

namespace vlux {
SyncObject::SyncObject(const VkDevice device) : device_(device) {
    const auto kSemaphoreInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    const auto kFenceInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (vkCreateSemaphore(device, &kSemaphoreInfo, nullptr, &image_available_semaphore_) !=
            VK_SUCCESS ||
        vkCreateSemaphore(device, &kSemaphoreInfo, nullptr, &render_finished_semaphore_) !=
            VK_SUCCESS ||
        vkCreateFence(device, &kFenceInfo, nullptr, &in_flight_fence_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
}

SyncObject::~SyncObject() {
    vkDestroySemaphore(device_, render_finished_semaphore_, nullptr);
    vkDestroySemaphore(device_, image_available_semaphore_, nullptr);
    vkDestroyFence(device_, in_flight_fence_, nullptr);
}

void SyncObject::WaitAndResetFences() const {
    vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &in_flight_fence_);
}

}  // namespace vlux