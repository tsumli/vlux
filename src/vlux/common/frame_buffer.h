#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "pch.h"

namespace vlux {
class FrameBuffer {
   public:
    FrameBuffer() = delete;
    FrameBuffer(const VkDevice device, const VkFramebufferCreateInfo framebuffer_info)
        : device_(device) {
        if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffer_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
    ~FrameBuffer() { vkDestroyFramebuffer(device_, framebuffer_, nullptr); }
    FrameBuffer(const FrameBuffer&) = default;
    FrameBuffer& operator=(const FrameBuffer&) = default;

    // accessor
    VkFramebuffer GetVkFrameBuffer() const { return framebuffer_; }

   private:
    VkDevice device_;
    VkFramebuffer framebuffer_;
};
}  // namespace vlux
#endif