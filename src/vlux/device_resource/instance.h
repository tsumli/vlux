#ifndef INSTANCE_H
#define INSTANCE_H
#include "pch.h"

namespace vlux {
class Instance {
   public:
    Instance();
    ~Instance() { vkDestroyInstance(instance_, nullptr); }

    VkInstance GetVkInstance() const { return instance_; }

   private:
    VkInstance instance_ = VK_NULL_HANDLE;
};
}  // namespace vlux

#endif