#include "queue.h"

namespace vlux {

QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    auto queue_families = std::vector<VkQueueFamilyProperties>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    QueueFamilyIndices indices;
    for (auto i = 0uz; i < queue_families.size(); i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }

        auto present_support = VkBool32(false);
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support) {
            indices.present_family = i;
        }

        if (indices.IsComplete()) {
            break;
        }
    }

    return indices;
}

std::pair<VkQueue, VkQueue> CreateQueue(const VkDevice device,
                                        const VkPhysicalDevice physical_device,
                                        const VkSurfaceKHR surface) {
    const auto indices = FindQueueFamilies(physical_device, surface);
    VkQueue graphics_queue;
    VkQueue present_queue;
    vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
    return {graphics_queue, present_queue};
}

}  // namespace vlux