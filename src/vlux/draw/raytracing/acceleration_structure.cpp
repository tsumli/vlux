#include "acceleration_structure.h"

#include "common/utils.h"

namespace vlux::draw::raytracing {
AccelerationStructure::AccelerationStructure(
    const VkDevice device, const VkPhysicalDevice physical_device,
    const VkAccelerationStructureBuildSizesInfoKHR build_size_info)
    : device_(device) {
    const auto buffer_create_info = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = build_size_info.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer for acceleration structure");
    }

    const auto memory_requirements_info = VkBufferMemoryRequirementsInfo2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = nullptr,
        .buffer = buffer_,
    };
    auto memory_requirements = VkMemoryRequirements2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = nullptr,
    };
    // VkAccelerationStructureMemoryRequirementsInfoKHR{};
    vkGetBufferMemoryRequirements2(device, &memory_requirements_info, &memory_requirements);
    const auto memory_allocate_frags_info = VkMemoryAllocateFlagsInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
    };
    const auto memory_allocate_info = VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &memory_allocate_frags_info,
        .allocationSize = memory_requirements.memoryRequirements.size,
        .memoryTypeIndex = FindMemoryType(memory_requirements.memoryRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device),
    };
    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory for acceleration structure");
    }
    if (vkBindBufferMemory(device, buffer_, memory_, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind memory to acceleration structure buffer");
    }
}

AccelerationStructure::~AccelerationStructure() {
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, buffer_, nullptr);
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memory_, nullptr);
    }
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
        vkGetDeviceProcAddr(device_, "vkDestroyAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR(device_, handle_, nullptr);
}
}  // namespace vlux::draw::raytracing
