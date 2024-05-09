#ifndef TEXTURE_SAMPLER_H
#define TEXTURE_SAMPLER_H
#include "pch.h"

namespace vlux {
class TextureSampler {
   public:
    TextureSampler(const VkPhysicalDevice physical_device, const VkDevice device)
        : device_(device) {
        auto raytracing_properties = VkPhysicalDeviceRayTracingPipelinePropertiesKHR{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
        };
        auto properties = VkPhysicalDeviceProperties2{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &raytracing_properties,
        };
        vkGetPhysicalDeviceProperties2(physical_device, &properties);

        const auto sampler_info = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = properties.properties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };
        if (vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }
    TextureSampler(const TextureSampler&) = delete;
    TextureSampler& operator=(const TextureSampler&) = delete;
    TextureSampler(TextureSampler&&) = default;
    TextureSampler& operator=(TextureSampler&&) = default;
    ~TextureSampler() { vkDestroySampler(device_, texture_sampler_, nullptr); }

    VkSampler GetSampler() const { return texture_sampler_; }

   private:
    VkDevice device_;
    VkSampler texture_sampler_;
};
}  // namespace vlux

#endif