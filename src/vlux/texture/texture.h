#ifndef TEXTURE_H
#define TEXTURE_H
#include <vulkan/vulkan_core.h>

#include "pch.h"
//
#include "common/buffer.h"
#include "common/command_buffer.h"
#include "common/image.h"

namespace vlux {

template <PixelType T>
class Texture {
   public:
    Texture(const Image<T>& image, const VkQueue graphics_queue, const VkCommandPool command_pool,
            const VkDevice device, const VkPhysicalDevice physical_device,
            const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM)
        : device_(device) {
        buffer_.emplace(device, physical_device, image.GetWidth(), image.GetHeight(), format,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, VK_IMAGE_ASPECT_COLOR_BIT);

        // Transition
        [&]() {
            auto command_buffer = BeginSingleTimeCommands(command_pool, device);
            const auto barrier = VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = buffer_->GetVkImage(),
                .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &barrier);

            EndSingleTimeCommands(command_buffer, graphics_queue, command_pool, device);
        }();

        const auto staging_buffer =
            Buffer(device, physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   image.GetSize(), image.GetPixels());

        // copy buffer to image
        [&]() {
            const auto command_buffer = BeginSingleTimeCommands(command_pool, device);

            const auto region = VkBufferImageCopy{
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .imageOffset = {0, 0, 0},
                .imageExtent =
                    {
                        .width = static_cast<uint32_t>(image.GetWidth()),
                        .height = static_cast<uint32_t>(image.GetHeight()),
                        .depth = 1,
                    },
            };
            vkCmdCopyBufferToImage(command_buffer, staging_buffer.GetVkBuffer(),
                                   buffer_->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &region);
            EndSingleTimeCommands(command_buffer, graphics_queue, command_pool, device);
        }();

        // Transition
        [&]() {
            auto command_buffer = BeginSingleTimeCommands(command_pool, device);
            const auto barrier = VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dstAccessMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = buffer_->GetVkImage(),
                .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &barrier);

            EndSingleTimeCommands(command_buffer, graphics_queue, command_pool, device);
        }();
    }

    ~Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    VkImageView GetImageView() const { return buffer_->GetVkImageView(); }

   private:
    VkDevice device_;
    std::optional<ImageBuffer> buffer_;
};

}  // namespace vlux

#endif