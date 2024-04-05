#include "descriptor_set.h"

#include "texture/texture_sampler.h"
#include "texture_view.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {

DescriptorSet::DescriptorSet(const VkPhysicalDevice physical_device, const VkDevice device,
                             const VkDescriptorPool descriptor_pool,
                             const VkDescriptorSetLayout descriptor_set_layout,
                             const UniformBuffer<TransformParams>& transform_ubo,
                             const std::optional<TextureView> texture_view)
    : texture_sampler_(std::make_shared<TextureSampler>(physical_device, device)) {
    const auto layouts =
        std::vector<VkDescriptorSetLayout>(kMaxFramesInFlight, descriptor_set_layout);
    const auto alloc_info = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };

    descriptor_sets_.resize(kMaxFramesInFlight);
    if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < kMaxFramesInFlight; i++) {
        const auto transform_ubo_buffer_info = VkDescriptorBufferInfo{
            .buffer = transform_ubo.GetVkUniformBuffer(i),
            .offset = 0,
            .range = transform_ubo.GetUniformBufferObjectSize(),
        };

        auto descriptor_write = std::vector<VkWriteDescriptorSet>();
        descriptor_write.emplace_back(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_sets_[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &transform_ubo_buffer_info,
        });
        if (texture_view.has_value()) {
            const auto create_image_info = [&](const VkImageView image_view) {
                return VkDescriptorImageInfo{
                    .sampler = texture_sampler_->GetSampler(),
                    .imageView = image_view,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
            };

            const auto color_image_info = create_image_info(texture_view->color);
            descriptor_write.emplace_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets_[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &color_image_info,
            });

            const auto normal_image_info = create_image_info(texture_view->normal);
            descriptor_write.emplace_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets_[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &normal_image_info,
            });
        }
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_write.size()),
                               descriptor_write.data(), 0, nullptr);
    }
}

}  // namespace vlux::draw::rasterize