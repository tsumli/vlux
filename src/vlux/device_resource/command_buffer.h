#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H
#include "pch.h"
//
#include "scene/scene.h"
namespace vlux {
class CommandBuffer {
   public:
    CommandBuffer() = delete;
    CommandBuffer(const VkCommandPool command_pool, const VkDevice device);

    // accessor
    const VkCommandBuffer& GetVkCommandBuffer() const { return command_buffer_; }

    void ResetCommandBuffer(
        VkCommandBufferResetFlagBits flags = static_cast<VkCommandBufferResetFlagBits>(0)) {
        vkResetCommandBuffer(command_buffer_, flags);
    }

   private:
    const VkDevice device_;
    VkCommandBuffer command_buffer_;
};
}  // namespace vlux

#endif