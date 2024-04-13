#ifndef COMMON_RENDER_PASS_H
#define COMMON_RENDER_PASS_H

#include "pch.h"

namespace vlux {
class RenderPass {
   public:
    RenderPass() = delete;
    RenderPass(const VkDevice device, const VkRenderPassCreateInfo render_pass_info)
        : device_(device) {
        if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }
    ~RenderPass() { vkDestroyRenderPass(device_, render_pass_, nullptr); }

    // accessor
    VkRenderPass GetVkRenderPass() const {
        if (render_pass_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::render_pass_` has no values.");
        }
        return render_pass_;
    }

   private:
    const VkDevice device_;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
};

}  // namespace vlux
#endif