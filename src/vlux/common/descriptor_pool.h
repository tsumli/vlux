#ifndef COMMON_DESCRIPTOR_POOL_H
#define COMMON_DESCRIPTOR_POOL_H

#include "pch.h"

namespace vlux {
class DescriptorPool {
   public:
    DescriptorPool(const VkDevice device, const VkDescriptorPoolCreateInfo pool_info)
        : device_(device) {
        if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
    ~DescriptorPool() { vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr); }

    VkDescriptorPool GetVkDescriptorPool() const { return descriptor_pool_; }

   private:
    const VkDevice device_;
    VkDescriptorPool descriptor_pool_;
};
}  // namespace vlux
#endif