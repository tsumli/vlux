#ifndef COMMON_DESCRIPTOR_SET_LAYOUT_H
#define COMMON_DESCRIPTOR_SET_LAYOUT_H

#include <vulkan/vulkan_core.h>

#include "pch.h"

namespace vlux {
class DescriptorSetLayout {
   public:
    DescriptorSetLayout(const VkDevice device, const VkDescriptorSetLayoutCreateInfo layout_info)
        : device_(device) {
        if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout_) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
    ~DescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
    }

    VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return descriptor_set_layout_; }

   private:
    const VkDevice device_;
    VkDescriptorSetLayout descriptor_set_layout_;
};
}  // namespace vlux
#endif