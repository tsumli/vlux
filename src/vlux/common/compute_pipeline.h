#ifndef COMMON_COMPUTE_PIPELINE_H
#define COMMON_COMPUTE_PIPELINE_H

#include <vulkan/vulkan_core.h>

#include "pch.h"

namespace vlux {
class ComputePipeline {
   public:
    ComputePipeline() = delete;
    ComputePipeline(const VkDevice device, const VkComputePipelineCreateInfo pipeline_info)
        : device_(device) {
        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                     &compute_pipeline_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }
    }

    ~ComputePipeline() { vkDestroyPipeline(device_, compute_pipeline_, nullptr); }

    // accessor
    VkPipeline GetVkComputePipeline() const {
        if (compute_pipeline_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::compute_pipeline_` is `VK_NULL_HANDLE`");
        }
        return compute_pipeline_;
    }

   private:
    const VkDevice device_;
    VkPipeline compute_pipeline_ = VK_NULL_HANDLE;
};
}  // namespace vlux
#endif