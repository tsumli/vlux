#include "descriptor_set_layout.h"

namespace vlux::draw::rasterize {
DescriptorSetLayout::DescriptorSetLayout(const VkDevice device) : device_(device) {
    constexpr auto kLayoutBindings = std::to_array({
        // transform
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        // color
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // TODO: separate image and sampler
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
        // normal
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    });

    const auto layout_info = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
        .pBindings = kLayoutBindings.data(),
    };

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout_) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}
DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
}
}  // namespace vlux::draw::rasterize