#include "index.h"

namespace vlux {
IndexBuffer::IndexBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                         const VkQueue queue, const VkCommandPool command_pool,
                         std::vector<Index>&& indices)
    : device_(device), index_count_(indices.size()) {
    const auto buffer_size = static_cast<VkDeviceSize>(sizeof(Index) * indices.size());
    const auto staging_buffer =
        Buffer(device_, physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               buffer_size, indices.data());

    buffer_.emplace(device_, physical_device,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer_size, nullptr);

    CopyBuffer(staging_buffer.GetVkBuffer(), buffer_->GetVkBuffer(), buffer_size, command_pool,
               queue, device_);
}

}  // namespace vlux