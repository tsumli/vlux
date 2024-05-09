#include "command_buffer.h"

namespace vlux {
VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool command_pool, const VkDevice device) {
    const auto alloc_info = VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    if (vkAllocateCommandBuffers(device, &alloc_info, &command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    const auto begin_info = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    return command_buffer;
}

void EndSingleTimeCommands(const VkCommandBuffer command_buffer, const VkQueue queue,
                           const VkCommandPool command_pool, const VkDevice device) {
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
    const auto command_buffer_submit_info = VkCommandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = command_buffer,
    };

    const auto submit_info = VkSubmitInfo2{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &command_buffer_submit_info,
    };
    if (vkQueueSubmit2(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit copy buffer command buffer!");
    }

    if (vkQueueWaitIdle(queue) != VK_SUCCESS) {
        throw std::runtime_error("failed to wait for queue to idle!");
    }

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

CommandBuffer::CommandBuffer(const VkCommandPool command_pool, const VkDevice device)
    : device_(device) {
    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

}  // namespace vlux