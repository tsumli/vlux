#include "index.h"

namespace vlux {
IndexBuffer::IndexBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                         const VkQueue queue, const VkCommandPool command_pool,
                         std::vector<Index>&& indices)
    : device_(device), indices_(std::move(indices)) {
    const auto buffer_size = static_cast<VkDeviceSize>(sizeof(Index) * indices_.size());

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging_buffer, staging_buffer_memory, device_, physical_device);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, indices_.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(device, staging_buffer_memory);

    CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer_, index_buffer_memory_, device_,
                 physical_device);

    CopyBuffer(staging_buffer, index_buffer_, buffer_size, command_pool, queue, device_);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);
}

IndexBuffer::~IndexBuffer() {
    vkDestroyBuffer(device_, index_buffer_, nullptr);
    vkFreeMemory(device_, index_buffer_memory_, nullptr);
}

}  // namespace vlux