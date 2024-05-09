#ifndef DRAW_RAYTRACING_ACCELERATION_STRUCTURE_H
#define DRAW_RAYTRACING_ACCELERATION_STRUCTURE_H

#include "pch.h"

namespace vlux::draw::raytracing {
class AccelerationStructure {
   public:
    AccelerationStructure() = default;
    AccelerationStructure(const VkDevice device, const VkPhysicalDevice physical_device,
                          const VkAccelerationStructureBuildSizesInfoKHR build_size_info);
    ~AccelerationStructure();

    VkBuffer GetBuffer() const { return buffer_; }
    VkAccelerationStructureKHR GetHandle() const { return handle_; }
    VkAccelerationStructureKHR& MutableHandle() { return handle_; }
    void SetDeviceAddress(const uint64_t device_address) { device_address_ = device_address; }
    uint64_t GetDeviceAddress() const { return device_address_; }

   private:
    VkDevice device_;
    VkAccelerationStructureKHR handle_;
    uint64_t device_address_;
    VkDeviceMemory memory_;
    VkBuffer buffer_;

    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
};

}  // namespace vlux::draw::raytracing

#endif