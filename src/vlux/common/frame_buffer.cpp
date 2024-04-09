#include "frame_buffer.h"

namespace vlux {

FrameBuffer::FrameBuffer(const VkRenderPass render_pass, const VkDevice device,
                         const uint32_t width, const uint32_t height,
                         const std::vector<VkImageView>& image_views,
                         const VkImageView depth_image_view)
    : device_(device) {
    framebuffers_.resize(image_views.size());
    for (auto i = 0uz; i < image_views.size(); i++) {
        const auto attachments = std::to_array({image_views[i], depth_image_view});
        const auto framebuffer_info = VkFramebufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = width,
            .height = height,
            .layers = 1,
        };
        if (vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &framebuffers_[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

FrameBuffer::~FrameBuffer() {
    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
}

}  // namespace vlux