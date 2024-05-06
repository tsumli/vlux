#ifndef COMMON_IMAGE_H
#define COMMON_IMAGE_H

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <limits>

#include "pch.h"

namespace vlux {
void CreateImage(const uint32_t width, const uint32_t height, VkFormat format,
                 const VkImageTiling tiling, const VkImageUsageFlags usage,
                 const VkMemoryPropertyFlags properties, VkImage& image,
                 VkDeviceMemory& image_memory, const VkDevice device,
                 const VkPhysicalDevice physical_device);

VkImageView CreateImageView(const VkImage image, const VkFormat format,
                            const VkImageViewCreateFlags create_flags,
                            VkImageAspectFlags aspect_flags, const VkDevice device);

class ImageBuffer {
   public:
    ImageBuffer(const VkDevice device, const VkPhysicalDevice physical_device, const uint32_t width,
                const uint32_t height, const VkFormat format, const VkImageTiling tiling,
                const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties,
                const VkImageViewCreateFlags create_flags, VkImageAspectFlags aspect_flags);

    ~ImageBuffer();

    VkImage GetVkImage() const { return image_; }
    VkImageView GetVkImageView() const { return image_view_; }
    VkFormat GetVkFormat() const { return format_; }

   private:
    VkDevice device_;
    VkPhysicalDevice physical_device_;
    uint32_t width_;
    uint32_t height_;

    VkFormat format_;
    VkImage image_;
    VkDeviceMemory image_memory_;
    VkImageView image_view_;
};

// concept for uint8_t or float
template <typename T>
concept PixelType = std::is_same_v<T, uint8_t> || std::is_same_v<T, float>;

template <PixelType T>
class Image {
   public:
    Image(const std::vector<T>& pixels, const int width, const int height, const int channels)
        : pixels_(pixels), width_(width), height_(height), channels_(channels) {
        assert(static_cast<size_t>(height_ * width_ * channels_) == pixels.size());
    }

    ~Image() = default;

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    VkDeviceSize GetSize() const { return width_ * height_ * 4; }
    const T* GetPixels() const { return pixels_.data(); }

   private:
    std::vector<T> pixels_;
    int width_;
    int height_;
    int channels_;
};
}  // namespace vlux

#endif