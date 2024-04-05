#ifndef INDEX_H
#define INDEX_H

#include "pch.h"
//
#include "device_resource/buffer.h"

namespace vlux {
using Index = uint16_t;

class IndexBuffer {
   public:
    IndexBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                const VkQueue graphics_queue, const VkCommandPool command_pool,
                std::vector<Index>&& indices);
    ~IndexBuffer();

    VkBuffer GetIndexBuffer() const { return index_buffer_; }
    size_t GetSize() const { return indices_.size(); }

   private:
    const VkDevice device_;
    const std::vector<Index> indices_;
    VkBuffer index_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory index_buffer_memory_ = VK_NULL_HANDLE;
};
}  // namespace vlux

#endif