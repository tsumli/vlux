#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "pch.h"

namespace vlux {
uint32_t FindMemoryType(const uint32_t type_filter, const VkMemoryPropertyFlags properties,
                        const VkPhysicalDevice physical_device);

}  // namespace vlux

#endif
