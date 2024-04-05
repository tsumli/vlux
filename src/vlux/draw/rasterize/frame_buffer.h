#ifndef RASTERIZE_FRAME_BUFFER_H
#define RASTERIZE_FRAME_BUFFER_H

#include "pch.h"

namespace vlux::draw::rasterize {
class FrameBuffer {
   public:
    FrameBuffer() = delete;
    FrameBuffer(const VkRenderPass render_pass, const VkDevice device, const uint32_t width,
                const uint32_t height, const std::vector<VkImageView>& image_views,
                const VkImageView depth_image_view);
    ~FrameBuffer();
    FrameBuffer(const FrameBuffer&) = default;
    FrameBuffer& operator=(const FrameBuffer&) = default;

    // accessor
    const std::vector<VkFramebuffer>& GetVkFrameBuffers() const { return framebuffers_; }
    const VkFramebuffer& GetVkFrameBuffer(const size_t idx) const { return framebuffers_[idx]; }

   private:
    VkDevice device_;
    std::vector<VkFramebuffer> framebuffers_;
};
}  // namespace vlux::draw::rasterize
#endif