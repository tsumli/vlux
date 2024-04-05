#ifndef RASTERIZE_DESCRIPTOR_POOL_H
#define RASTERIZE_DESCRIPTOR_POOL_H

#include "pch.h"

namespace vlux::draw::rasterize {
class DescriptorPool {
   public:
    DescriptorPool(const VkDevice device, const uint32_t num_model);
    ~DescriptorPool();

    VkDescriptorPool GetVkDescriptorPool() const { return descriptor_pool_; }

   private:
    const VkDevice device_;
    VkDescriptorPool descriptor_pool_;
};
}  // namespace vlux::draw::rasterize
#endif