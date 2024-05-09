#include "utils.h"

namespace vlux {
std::pair<int, int> GetWindowSize(GLFWwindow* window) {
    int width;
    int height;
    glfwGetWindowSize(window, &width, &height);

    return {width, height};
}

VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling,
                             const VkFormatFeatureFlags features,
                             const VkPhysicalDevice physical_device) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

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

    throw std::runtime_error(
        fmt::format("failed to find suitable memory type! type_filter: {}, properties: {}",
                    type_filter, properties));
}
}  // namespace vlux