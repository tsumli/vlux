#ifndef COMMON_IMAGE_H
#define COMMON_IMAGE_H

#include <vulkan/vulkan_core.h>

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

template <typename T>
concept IsUint8OrFloat = std::is_same_v<T, uint8_t> || std::is_same_v<T, float>;

template <IsUint8OrFloat T>
class Image {
   public:
    Image(const std::filesystem::path& path) {
        auto pixels = [&]() {
            if constexpr (std::is_same_v<T, uint8_t>) {
                return stbi_load(path.string().c_str(), &width_, &height_, &channels_,
                                 STBI_rgb_alpha);
            } else if constexpr (std::is_same_v<T, float>) {
                return stbi_loadf(path.string().c_str(), &width_, &height_, &channels_,
                                  STBI_rgb_alpha);
            }
        }();

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        pixels_ = std::vector<T>(pixels, pixels + width_ * height_ * 4);
        stbi_image_free(pixels);
    }

    Image(const std::vector<T>& pixels, const int width, const int height, const int channels)
        : pixels_(pixels), width_(width), height_(height), channels_(channels) {
        assert(static_cast<size_t>(height_ * width_ * channels_) == pixels.size());
    }

    ~Image() = default;

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    VkDeviceSize GetSize() const { return width_ * height_ * 4; }
    const uint8_t* GetPixels() const { return pixels_.data(); }

   private:
    std::vector<T> pixels_;
    int width_;
    int height_;
    int channels_;
};

}  // namespace vlux

#endif