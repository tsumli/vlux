#ifndef COMMON_RENDER_TARGET_H
#define COMMON_RENDER_TARGET_H

#include "pch.h"

class RenderTarget {
   public:
    explicit RenderTarget(const VkDevice device) : device_(device) {}
    ~RenderTarget() {
        vkDestroyImageView(device_, image_view_, nullptr);
        vkDestroyImage(device_, image_, nullptr);
        vkFreeMemory(device_, device_memory_, nullptr);
    }
    RenderTarget() = delete;
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&&) = delete;
    RenderTarget& operator=(RenderTarget&&) = delete;

    VkFormat GetFormat() const { return format_; }
    VkImage& GetImageRef() { return image_; }
    VkDeviceMemory& GetDeviceMemoryRef() { return device_memory_; }
    VkImageView GetImageView() const { return image_view_; }

    void SetFormat(const VkFormat format) { format_ = format; }
    void SetImageView(const VkImageView image_view) { image_view_ = image_view; }

   private:
    VkFormat format_;
    VkImage image_;
    VkDeviceMemory device_memory_;
    VkImageView image_view_;
    VkDevice device_;
};

#endif