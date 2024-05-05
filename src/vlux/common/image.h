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

void CopyBufferToImage(const VkBuffer buffer, const VkImage image, const uint32_t width,
                       const uint32_t height, const VkQueue graphics_queue,
                       const VkCommandPool command_pool, const VkDevice device);

// concept for uint8_t or float
template <typename T>
concept PixelType = std::is_same_v<T, uint8_t> || std::is_same_v<T, float>;

template <PixelType T>
class Image {
   public:
<<<<<<< HEAD
    Image(const std::vector<uint8_t>& pixels, const int width, const int height, const int channels)
||||||| constructed merge base
    Image(const std::filesystem::path& path) {
        auto pixels =
            stbi_load(path.string().c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        pixels_ = std::vector<uint8_t>(pixels, pixels + width_ * height_ * 4);
        stbi_image_free(pixels);
    }

    Image(const std::vector<uint8_t>& pixels, const int width, const int height, const int channels)
=======
    template <typename U = T>
    Image(const std::filesystem::path& path,
          std::enable_if_t<std::is_same_v<U, uint8_t>, int> = 0) {
        auto pixels =
            stbi_load(path.string().c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        pixels_ = std::vector<U>(pixels, pixels + width_ * height_ * 4);
        stbi_image_free(pixels);
    }

    Image(const std::vector<T>& pixels, const int width, const int height, const int channels)
>>>>>>> add: texture for float
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