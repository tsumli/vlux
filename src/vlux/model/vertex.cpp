#include "vertex.h"

#include "common/buffer.h"

namespace vlux {
VertexBuffer::VertexBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                           const VkQueue graphics_queue, const VkCommandPool command_pool,
                           std::vector<Vertex>&& vertices)
    : device_(device) {
    const auto buffer_size = static_cast<VkDeviceSize>(sizeof(Vertex) * vertices.size());
    const auto staging_buffer =
        Buffer(device_, physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               buffer_size, vertices.data());

    buffer_.emplace(device_, physical_device,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer_size, nullptr);

    CopyBuffer(staging_buffer.GetVkBuffer(), buffer_->GetVkBuffer(), buffer_size, command_pool,
               graphics_queue, device_);
}

}  // namespace vlux