#ifndef DRAW_RAYTRACING_SCRATCH_BUFFER_H
#define DRAW_RAYTRACING_SCRATCH_BUFFER_H

#include "pch.h"

namespace vlux::draw::raytracing {
class RayTracingScratchBuffer {
   public:
    RayTracingScratchBuffer() = default;
    RayTracingScratchBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                            const VkDeviceSize size);
    ~RayTracingScratchBuffer();

    uint64_t GetDeviceAddress() const { return device_address_; }

   private:
    VkDevice device_;
    uint64_t device_address_;
    VkBuffer handle_;
    VkDeviceMemory memory_;

    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
};

}  // namespace vlux::draw::raytracing

#endif