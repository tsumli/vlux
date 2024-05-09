#include "queue.h"

namespace vlux {

QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    auto queue_families = std::vector<VkQueueFamilyProperties>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    QueueFamilyIndices indices;
    for (auto i = 0uz; i < queue_families.size(); i++) {
        if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.graphics_compute_family = i;
        }

        auto present_support = VkBool32(false);
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support) {
            indices.present_family = i;
        }

        if (IsQueueFamilyIndicesComplete(indices)) {
            break;
        }
    }

    return indices;
}

Queues CreateQueue(const VkDevice device, const VkPhysicalDevice physical_device,
                   const VkSurfaceKHR surface) {
    const auto indices = FindQueueFamilies(physical_device, surface);

    VkQueue graphics_queue;
    vkGetDeviceQueue(device, indices.graphics_compute_family.value(), 0, &graphics_queue);

    VkQueue present_queue;
    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);

    return {
        .graphics_compute = graphics_queue,
        .present = present_queue,
    };
}

}  // namespace vlux