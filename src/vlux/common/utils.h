#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "pch.h"

namespace vlux {
VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling,
                             const VkFormatFeatureFlags features,
                             const VkPhysicalDevice physical_device);

uint32_t FindMemoryType(const uint32_t type_filter, const VkMemoryPropertyFlags properties,
                        const VkPhysicalDevice physical_device);

}  // namespace vlux

#endif
