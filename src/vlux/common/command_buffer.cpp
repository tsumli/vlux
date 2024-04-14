#include "command_buffer.h"

namespace vlux {
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