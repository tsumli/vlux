#ifndef COMMON_PIPELINE_LAYOUT_H
#define COMMON_PIPELINE_LAYOUT_H

#include <vulkan/vulkan_core.h>

#include "pch.h"

namespace vlux {

class PipelineLayout {
   public:
    PipelineLayout(const VkDevice device, const VkPipelineLayoutCreateInfo pipeline_layout_info)
        : device_(device) {
        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout_) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    ~PipelineLayout() { vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr); }

    VkPipelineLayout GetVkPipelineLayout() const {
        if (pipeline_layout_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::pipeline_layout_` is `VK_NULL_HANDLE`");
        }
        return pipeline_layout_;
    }

   private:
    const VkDevice device_;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
};

}  // namespace vlux

#endif