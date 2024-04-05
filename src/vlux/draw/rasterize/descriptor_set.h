#ifndef RASTERIZE_DESCRIPTOR_SET_H
#define RASTERIZE_DESCRIPTOR_SET_H

#include "pch.h"
//
#include "texture/texture_sampler.h"
#include "texture_view.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {
class DescriptorSet {
   public:
    DescriptorSet(const VkPhysicalDevice physical_device, const VkDevice device,
                  const VkDescriptorPool descriptor_pool,
                  const VkDescriptorSetLayout descriptor_set_layout,
                  const UniformBuffer<TransformParams>& transform_ubo,
                  const std::optional<TextureView> texture_view = std::nullopt);
    VkDescriptorSet GetVkDescriptorSet(const size_t idx) const { return descriptor_sets_.at(idx); }

   private:
    std::shared_ptr<TextureSampler> texture_sampler_;
    std::vector<VkDescriptorSet> descriptor_sets_;
};
}  // namespace vlux::draw::rasterize
#endif