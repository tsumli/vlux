#include "utils.h"

namespace vlux {
uint32_t FindMemoryType(const uint32_t type_filter, const VkMemoryPropertyFlags properties,
                        const VkPhysicalDevice physical_device) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
}  // namespace vlux