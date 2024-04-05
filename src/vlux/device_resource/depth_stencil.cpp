#include "depth_stencil.h"

namespace vlux {
namespace {
VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling,
                             const VkFormatFeatureFlags features,
                             const VkPhysicalDevice physical_device) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat FindDepthFormat(const VkPhysicalDevice physical_device) {
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, physical_device);
}
}  // namespace

DepthStencil::DepthStencil(const int width, const int height, const VkDevice device,
                           const VkPhysicalDevice physical_device)
    : device_(device), depth_format_(FindDepthFormat(physical_device)) {
    CreateImage(width, height, depth_format_, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depth_image_, depth_image_memory_, device, physical_device);
    depth_image_view_ =
        CreateImageView(depth_image_, depth_format_, VK_IMAGE_ASPECT_DEPTH_BIT, device);
}

DepthStencil::~DepthStencil() {
    vkDestroyImageView(device_, depth_image_view_, nullptr);
    vkDestroyImage(device_, depth_image_, nullptr);
    vkFreeMemory(device_, depth_image_memory_, nullptr);
}

}  // namespace vlux