#ifndef COMMON_BUFFER_H
#define COMMON_BUFFER_H

#include "pch.h"
//
#include "queue.h"

namespace vlux {
void CreateBuffer(const VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory,
                  const VkDevice device, const VkPhysicalDevice physical_device);

void CopyBuffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size,
                const VkCommandPool command_pool, const VkQueue graphics_queue,
                const VkDevice device);
}  // namespace vlux

#endif