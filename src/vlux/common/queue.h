#ifndef COMMON_QUEUE_H
#define COMMON_QUEUE_H
#include "pch.h"

namespace vlux {

struct Queues {
    VkQueue graphics_compute;
    VkQueue present;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_compute_family;
    std::optional<uint32_t> present_family;
};

inline bool IsQueueFamilyIndicesComplete(const QueueFamilyIndices& indices) {
    return indices.graphics_compute_family.has_value() && indices.present_family.has_value();
}

QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface);

Queues CreateQueue(const VkDevice device, const VkPhysicalDevice physical_device,
                   const VkSurfaceKHR surface);

}  // namespace vlux

#endif