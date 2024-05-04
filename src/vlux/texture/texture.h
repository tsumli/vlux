#ifndef TEXTURE_H
#define TEXTURE_H
#include <vulkan/vulkan_core.h>

#include "pch.h"
//
#include "common/buffer.h"
#include "common/image.h"
#include "device_resource/barrier.h"

namespace vlux {

class Texture {
   public:
    Texture(const Image& image, const VkQueue graphics_queue, const VkCommandPool command_pool,
            const VkDevice device, const VkPhysicalDevice physical_device)
        : device_(device) {
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        const auto format = VK_FORMAT_R8G8B8A8_UNORM;
        CreateBuffer(image.GetSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging_buffer, staging_buffer_memory, device, physical_device);

        void* data;
        vkMapMemory(device, staging_buffer_memory, 0, image.GetSize(), 0, &data);
        memcpy(data, image.GetPixels(), static_cast<size_t>(image.GetSize()));
        vkUnmapMemory(device, staging_buffer_memory);

        CreateImage(image.GetWidth(), image.GetHeight(), format, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image_, texture_image_memory_,
                    device, physical_device);

        TransitionImageLayout(texture_image_, format, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, graphics_queue, command_pool,
                              device);
        CopyBufferToImage(staging_buffer, texture_image_, static_cast<uint32_t>(image.GetWidth()),
                          static_cast<uint32_t>(image.GetHeight()), graphics_queue, command_pool,
                          device);
        TransitionImageLayout(texture_image_, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, graphics_queue,
                              command_pool, device);

        vkDestroyBuffer(device, staging_buffer, nullptr);
        vkFreeMemory(device, staging_buffer_memory, nullptr);

        texture_image_view_ =
            CreateImageView(texture_image_, format, 0, VK_IMAGE_ASPECT_COLOR_BIT, device);
    }

    ~Texture() {
        vkDestroyImageView(device_, texture_image_view_, nullptr);
        vkDestroyImage(device_, texture_image_, nullptr);
        vkFreeMemory(device_, texture_image_memory_, nullptr);
    }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    VkImageView GetImageView() const { return texture_image_view_; }

   private:
    VkDevice device_;
    VkImage texture_image_;
    VkDeviceMemory texture_image_memory_;
    VkImageView texture_image_view_;
};

}  // namespace vlux

#endif