#ifndef DEVICE_H
#define DEVICE_H
#include "pch.h"
//
#include "common/queue.h"

namespace vlux {
bool CheckDeviceExtensionSupport(VkPhysicalDevice physical_device);

bool IsDeviceSuitable(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface);

VkDevice CreateLogicalDevice(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface);

VkPhysicalDevice PickPhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface);

class Device {
   public:
    Device(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface);

    ~Device() { vkDestroyDevice(device_, nullptr); }

    VkDevice GetVkDevice() const { return device_; }

   private:
    VkDevice device_ = VK_NULL_HANDLE;
};

}  // namespace vlux

#endif