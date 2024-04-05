#ifndef RASTERIZE_GRAPICS_PIPELINE_H
#define RASTERIZE_GRAPICS_PIPELINE_H

#include "pch.h"

namespace vlux::draw::rasterize {
class GraphicsPipeline {
   public:
    GraphicsPipeline() = delete;
    GraphicsPipeline(const VkDevice device, const VkRenderPass render_pass,
                     const VkDescriptorSetLayout descriptor_set_layout);

    ~GraphicsPipeline();

    // accessor
    VkPipeline GetVkGraphicsPipeline() const {
        if (graphics_pipeline_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::graphics_pipeline_` is `VK_NULL_HANDLE`");
        }
        return graphics_pipeline_;
    }

    VkPipelineLayout GetVkPipelineLayout() const {
        if (pipeline_layout_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::pipeline_layout_` is `VK_NULL_HANDLE`");
        }
        return pipeline_layout_;
    }

   private:
    const VkDevice device_;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;
};
}  // namespace vlux::draw::rasterize
#endif