#ifndef BARRIER_H
#define BARRIER_H
#include "pch.h"

namespace vlux {
VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool command_pool, const VkDevice device);

void EndSingleTimeCommands(const VkCommandBuffer command_buffer, const VkQueue graphics_queue,
                           const VkCommandPool command_pool, const VkDevice device);

void TransitionImageLayout(VkImage image, [[maybe_unused]] VkFormat format,
                           VkImageLayout old_layout, VkImageLayout new_layout,
                           const VkQueue graphics_queue, const VkCommandPool command_pool,
                           const VkDevice device);
}  // namespace vlux

#endif
