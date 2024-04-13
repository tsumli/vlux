#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include "pch.h"

namespace vlux {
class DescriptorSets {
   public:
    DescriptorSets(const VkDevice device, const VkDescriptorSetAllocateInfo& alloc_info) {
        if (vkAllocateDescriptorSets(device, &alloc_info, &descriptor_sets_) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }
    ~DescriptorSets() = default;
    DescriptorSets(const DescriptorSets&) = default;
    DescriptorSets& operator=(const DescriptorSets&) = default;

    VkDescriptorSet GetVkDescriptorSet() const { return descriptor_sets_; }

   private:
    VkDescriptorSet descriptor_sets_;
};
}  // namespace vlux
#endif