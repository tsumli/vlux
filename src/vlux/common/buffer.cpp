#include "buffer.h"

#include <vulkan/vulkan_core.h>

#include "utils.h"

namespace vlux {

namespace {
void FlushMappedMemory(const VkDeviceMemory buffer_memory, const VkDevice device,
                       const VkDeviceSize size) {
    const auto mapped_range = VkMappedMemoryRange{
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = buffer_memory,
        .offset = 0,
        .size = size,
    };
    vkFlushMappedMemoryRanges(device, 1, &mapped_range);
}
}  // namespace

Buffer::Buffer(const VkDevice device, const VkPhysicalDevice physical_device,
               const VkBufferUsageFlags usage_flags, const VkMemoryPropertyFlags memory_properties,
               const VkDeviceSize size, const void* data)
    : device_(device), usage_flags_(usage_flags), memory_properties_(memory_properties) {
    const auto buffer_info = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer_, &mem_requirements);

    const auto alloc_flags_info = [&]() -> std::optional<VkMemoryAllocateFlagsInfoKHR> {
        if (usage_flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            VkMemoryAllocateFlagsInfoKHR info;
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            return info;
        } else {
            return {};
        }
    }();

    const auto alloc_info = VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = alloc_flags_info.has_value() ? &alloc_flags_info.value() : nullptr,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex =
            FindMemoryType(mem_requirements.memoryTypeBits, memory_properties, physical_device),
    };

    if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    if (data != nullptr) {
        vkMapMemory(device, buffer_memory_, 0, size, 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(size));
        if ((memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
            FlushMappedMemory(buffer_memory_, device, size);
        }
        vkUnmapMemory(device, buffer_memory_);
    }

    vkBindBufferMemory(device, buffer_, buffer_memory_, 0);
}

Buffer::~Buffer() {
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, buffer_, nullptr);
    }
    if (buffer_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, buffer_memory_, nullptr);
    }
}

void Buffer::UpdateBuffer(const void* data, const VkDeviceSize size) {
    if (mapped == nullptr) {
        throw std::runtime_error("failed to map buffer memory!");
    }
    memcpy(mapped, data, static_cast<size_t>(size));
    if ((memory_properties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        FlushMappedMemory(buffer_memory_, device_, size);
    }
}

void CreateBuffer(const VkDeviceSize size, VkBufferUsageFlags usage,
                  const VkMemoryPropertyFlags properties, VkBuffer& buffer,
                  VkDeviceMemory& buffer_memory, const VkDevice device,
                  const VkPhysicalDevice physical_device) {
    const auto buffer_info = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

    const auto alloc_info = VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex =
            FindMemoryType(mem_requirements.memoryTypeBits, properties, physical_device),
    };

    if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void CopyBuffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size,
                const VkCommandPool command_pool, const VkQueue queue, const VkDevice device) {
    const auto alloc_info = VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    const auto begin_info = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(command_buffer, &begin_info);

    const auto copy_region = VkBufferCopy{
        .size = size,
    };
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    const auto submit_info = VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}
}  // namespace vlux