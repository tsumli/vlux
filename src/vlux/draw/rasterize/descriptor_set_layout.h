#ifndef RASTERIZE_DESCRIPTOR_SET_LAYOUT_H
#define RASTERIZE_DESCRIPTOR_SET_LAYOUT_H

#include "pch.h"

namespace vlux::draw::rasterize {
class DescriptorSetLayout {
   public:
    DescriptorSetLayout(const VkDevice device);
    ~DescriptorSetLayout();

    VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return descriptor_set_layout_; }

   private:
    const VkDevice device_;
    VkDescriptorSetLayout descriptor_set_layout_;
};
}  // namespace vlux::draw::rasterize
#endif