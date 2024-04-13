#ifndef COMMON_GRAPICS_PIPELINE_H
#define COMMON_GRAPICS_PIPELINE_H

#include <vulkan/vulkan_core.h>

#include "pch.h"

namespace vlux {
class GraphicsPipeline {
   public:
    GraphicsPipeline() = delete;
    GraphicsPipeline(const VkDevice device, const VkGraphicsPipelineCreateInfo pipeline_info)
        : device_(device) {
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                      &graphics_pipeline_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

    ~GraphicsPipeline() { vkDestroyPipeline(device_, graphics_pipeline_, nullptr); }

    // accessor
    VkPipeline GetVkGraphicsPipeline() const {
        if (graphics_pipeline_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::graphics_pipeline_` is `VK_NULL_HANDLE`");
        }
        return graphics_pipeline_;
    }

   private:
    const VkDevice device_;
    VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;
};
}  // namespace vlux
#endif