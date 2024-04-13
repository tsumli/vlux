#include "vertex.h"

#include "common/buffer.h"

namespace vlux {
VertexBuffer::VertexBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                           const VkQueue graphics_queue, const VkCommandPool command_pool,
                           std::vector<Vertex>&& vertices)
    : device_(device), vertices_(std::move(vertices)) {
    const auto buffer_size = static_cast<VkDeviceSize>(sizeof(Vertex) * vertices_.size());

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging_buffer, staging_buffer_memory, device_, physical_device);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices_.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(device, staging_buffer_memory);

    CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer_, vertex_buffer_memory_,
                 device_, physical_device);

    CopyBuffer(staging_buffer, vertex_buffer_, buffer_size, command_pool, graphics_queue, device_);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);
}

VertexBuffer::~VertexBuffer() {
    vkDestroyBuffer(device_, vertex_buffer_, nullptr);
    vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
}

}  // namespace vlux