#ifndef COMMON_BUFFER_H
#define COMMON_BUFFER_H

#include "pch.h"

namespace vlux {

/**
 * @brief A wrapper class for VkBuffer and VkDeviceMemory
 */
class Buffer {
   public:
    Buffer() = delete;
    /**
     * @brief Construct a new Buffer object
     *
     * @param device
     * @param physical_device
     * @param usage_flags
     * @param memory_properties
     * @param size
     * @param data
     */
    Buffer(const VkDevice device, const VkPhysicalDevice physical_device,
           const VkBufferUsageFlags usage_flags, const VkMemoryPropertyFlags memory_properties,
           const VkDeviceSize size, const void* data);

    ~Buffer();

    VkBuffer GetVkBuffer() const { return buffer_; }
    VkDeviceSize GetSize() const { return size_; }

    void UpdateBuffer(const void* data, const VkDeviceSize size);

   private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory buffer_memory_ = VK_NULL_HANDLE;

    VkBufferUsageFlags usage_flags_;
    VkMemoryPropertyFlags memory_properties_;

    void* mapped = nullptr;

    VkDeviceSize size_ = 0;
};

void CopyBuffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size,
                const VkCommandPool command_pool, const VkQueue queue, const VkDevice device);
}  // namespace vlux

#endif