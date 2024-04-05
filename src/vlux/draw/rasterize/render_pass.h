#ifndef RASTERIZE_RENDER_PASS_H
#define RASTERIZE_RENDER_PASS_H

#include "pch.h"

namespace vlux::draw::rasterize {
class RenderPass {
   public:
    RenderPass() = delete;
    RenderPass(const VkDevice device, const VkFormat format,
               const VkAttachmentDescription depth_stencil_attachment,
               const VkAttachmentReference depth_stencil_attachment_ref);
    ~RenderPass();

    // accessor
    VkRenderPass GetVkRenderPass() const {
        if (!render_pass_.has_value()) {
            throw std::runtime_error("`DeviceResource::render_pass_` has no values.");
        }
        return render_pass_.value();
    }

   private:
    const VkDevice device_;
    std::optional<VkRenderPass> render_pass_ = std::nullopt;
};

}  // namespace vlux::draw::rasterize
#endif