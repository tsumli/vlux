#include "scratch_buffer.h"

#include "common/utils.h"

namespace vlux::draw::raytracing {

RayTracingScratchBuffer::RayTracingScratchBuffer(const VkDevice device,
                                                 const VkPhysicalDevice physical_device,
                                                 const VkDeviceSize size)
    : device_(device) {
    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
        vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));

    const auto buffer_create_info = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };

    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &handle_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer for scratch buffer");
    }

    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(device, handle_, &memory_requirements);

    const auto memory_allocate_flags_info = VkMemoryAllocateFlagsInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
    };

    const auto memory_allocate_info = VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &memory_allocate_flags_info,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device)};

    if (vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory for scratch buffer");
    }

    if (vkBindBufferMemory(device, handle_, memory_, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind memory to scratch buffer");
    }

    const auto buffer_device_address_info = VkBufferDeviceAddressInfoKHR{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = handle_,
    };
    device_address_ = vkGetBufferDeviceAddressKHR(device, &buffer_device_address_info);
}

RayTracingScratchBuffer::~RayTracingScratchBuffer() {
    if (handle_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, handle_, nullptr);
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memory_, nullptr);
    }
}
}  // namespace vlux::draw::raytracing
