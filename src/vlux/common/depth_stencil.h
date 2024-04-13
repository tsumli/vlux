#ifndef DEPTH_H
#define DEPTH_H

#include "pch.h"

namespace vlux {
class DepthStencil {
   public:
    DepthStencil(const int width, const int height, const VkDevice device,
                 const VkPhysicalDevice physical_device);
    ~DepthStencil();

    VkAttachmentDescription CreateAttachmentDesc() const {
        return {
            .format = depth_format_,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    VkAttachmentReference CreateAttachmentRef() const {
        return {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    VkImageView GetVkImageView() const { return depth_image_view_; }

   private:
    const VkDevice device_;
    const VkFormat depth_format_;
    VkImage depth_image_;
    VkDeviceMemory depth_image_memory_;
    VkImageView depth_image_view_;
};

}  // namespace vlux

#endif