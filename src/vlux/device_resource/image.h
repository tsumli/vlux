#ifndef IMAGE_H
#define IMAGE_H

#include "pch.h"

namespace vlux {
void CreateImage(const uint32_t width, const uint32_t height, VkFormat format,
                 const VkImageTiling tiling, const VkImageUsageFlags usage,
                 const VkMemoryPropertyFlags properties, VkImage& image,
                 VkDeviceMemory& image_memory, const VkDevice device,
                 const VkPhysicalDevice physical_device);

VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags,
                            const VkDevice device);

void CopyBufferToImage(const VkBuffer buffer, const VkImage image, const uint32_t width,
                       const uint32_t height, const VkQueue graphics_queue,
                       const VkCommandPool command_pool, const VkDevice device);

class Image {
   public:
    Image(const std::filesystem::path& path) {
        auto pixels =
            stbi_load(path.string().c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        pixels_ = std::vector<unsigned char>(pixels, pixels + width_ * height_ * 4);
        stbi_image_free(pixels);
    }

    Image(const std::vector<uint8_t>& pixels, const int width, const int height, const int channels)
        : pixels_(pixels), width_(width), height_(height), channels_(channels) {
        assert(static_cast<size_t>(height_ * width_ * channels_) == pixels.size());
    }

    ~Image() {}

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    VkDeviceSize GetSize() const { return width_ * height_ * 4; }
    const uint8_t* GetPixels() const { return pixels_.data(); }

   private:
    std::vector<uint8_t> pixels_;
    int width_;
    int height_;
    int channels_;
};

}  // namespace vlux

#endif