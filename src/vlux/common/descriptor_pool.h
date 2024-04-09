#ifndef DESCRIPTOR_POOL_H
#define DESCRIPTOR_POOL_H

#include "pch.h"

namespace vlux {
class DescriptorPool {
   public:
    DescriptorPool(const VkDevice device, const uint32_t num_model);
    ~DescriptorPool();

    VkDescriptorPool GetVkDescriptorPool() const { return descriptor_pool_; }

   private:
    const VkDevice device_;
    VkDescriptorPool descriptor_pool_;
};
}  // namespace vlux
#endif