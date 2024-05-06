#include "image.h"

#include "common/command_buffer.h"
#include "common/utils.h"

namespace vlux {
ImageBuffer::ImageBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                         const uint32_t width, const uint32_t height, const VkFormat format,
                         const VkImageTiling tiling, const VkImageUsageFlags usage,
                         const VkMemoryPropertyFlags properties,
                         const VkImageViewCreateFlags create_flags, VkImageAspectFlags aspect_flags)
    : device_(device), physical_device_(physical_device), width_(width), height_(height) {
    // create image
    const auto image_info = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(device, &image_info, nullptr, &image_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, image_, &mem_requirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex =
            FindMemoryType(mem_requirements.memoryTypeBits, properties, physical_device_),
    };

    if (vkAllocateMemory(device, &allocInfo, nullptr, &image_memory_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image_, image_memory_, 0);

    // create image view
    const auto view_info = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = create_flags,
        .image = image_,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =
            {
                .aspectMask = aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },

    };

    if (vkCreateImageView(device, &view_info, nullptr, &image_view_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

ImageBuffer::~ImageBuffer() {
    vkDestroyImageView(device_, image_view_, nullptr);
    vkDestroyImage(device_, image_, nullptr);
    vkFreeMemory(device_, image_memory_, nullptr);
}

void CreateImage(const uint32_t width, const uint32_t height, VkFormat format,
                 const VkImageTiling tiling, const VkImageUsageFlags usage,
                 const VkMemoryPropertyFlags properties, VkImage& image,
                 VkDeviceMemory& image_memory, const VkDevice device,
                 const VkPhysicalDevice physical_device) {
    const auto image_info = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, image, &mem_requirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex =
            FindMemoryType(mem_requirements.memoryTypeBits, properties, physical_device),
    };

    if (vkAllocateMemory(device, &allocInfo, nullptr, &image_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, image_memory, 0);
}

VkImageView CreateImageView(const VkImage image, const VkFormat format,
                            const VkImageViewCreateFlags create_flags,
                            VkImageAspectFlags aspect_flags, const VkDevice device) {
    const auto view_info = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = create_flags,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =
            {
                .aspectMask = aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },

    };

    VkImageView image_view;
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return image_view;
}

}  // namespace vlux