#include "descriptor_pool.h"

namespace vlux::draw::rasterize {
DescriptorPool::DescriptorPool(const VkDevice device, const uint32_t num_model) : device_(device) {
    const auto pool_sizes = std::to_array({
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * num_model,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * num_model * 2,
        },
    });
    const auto pool_size_count = static_cast<uint32_t>(pool_sizes.size());
    const auto pool_info = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = num_model * kMaxFramesInFlight,
        .poolSizeCount = pool_size_count,
        .pPoolSizes = pool_sizes.data(),
    };

    if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

DescriptorPool::~DescriptorPool() { vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr); }

}  // namespace vlux::draw::rasterize