#include "sync_object.h"

namespace vlux {
SyncObject::SyncObject(const VkDevice device) : device_(device) {
    const auto kSemaphoreInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    const auto kFenceInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    if (vkCreateSemaphore(device, &kSemaphoreInfo, nullptr, &image_available_semaphore_) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphore for a frame!");
    }
    if (vkCreateSemaphore(device, &kSemaphoreInfo, nullptr, &render_finished_semaphore_) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphore for a frame!");
    }
    if (vkCreateFence(device, &kFenceInfo, nullptr, &in_flight_fence_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create fence for a frame");
    }
}

SyncObject::~SyncObject() {
    vkDestroySemaphore(device_, render_finished_semaphore_, nullptr);
    vkDestroySemaphore(device_, image_available_semaphore_, nullptr);
    vkDestroyFence(device_, in_flight_fence_, nullptr);
}

void SyncObject::WaitAndResetFences() const {
    if (vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE,
                        std::numeric_limits<uint64_t>::max()) != VK_SUCCESS) {
        throw std::runtime_error("failed to wait for a fence!");
    }
    if (vkResetFences(device_, 1, &in_flight_fence_) != VK_SUCCESS) {
        throw std::runtime_error("failed to reset a fence!");
    }
}

}  // namespace vlux