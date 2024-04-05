#ifndef QUEUE_H
#define QUEUE_H
#include "pch.h"

namespace vlux {
struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool IsComplete() const { return graphics_family.has_value() && present_family.has_value(); }
};

QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface);

std::pair<VkQueue, VkQueue> CreateQueue(const VkDevice device,
                                        const VkPhysicalDevice physical_device,
                                        const VkSurfaceKHR surface);

}  // namespace vlux

#endif