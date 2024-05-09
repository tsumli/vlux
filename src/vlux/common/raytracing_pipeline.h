#ifndef RAYTRACING_PIPLEINE_H
#define RAYTRACING_PIPLEINE_H

#include "pch.h"

namespace vlux {
class RaytracingPipeline {
   public:
    RaytracingPipeline() = delete;
    RaytracingPipeline(const VkDevice device, const VkRayTracingPipelineCreateInfoKHR pipeline_info)
        : device_(device) {
        vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
            vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
        if (vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
                                           &pipeline_info, nullptr,
                                           &raytracing_pipeline_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }
    }

    ~RaytracingPipeline() { vkDestroyPipeline(device_, raytracing_pipeline_, nullptr); }

    // accessor
    VkPipeline GetVkRaytracingPipeline() const {
        if (raytracing_pipeline_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::compute_pipeline_` is `VK_NULL_HANDLE`");
        }
        return raytracing_pipeline_;
    }

   private:
    const VkDevice device_;
    VkPipeline raytracing_pipeline_ = VK_NULL_HANDLE;

    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
};
}  // namespace vlux
#endif  // RAYTRACING_PIPLEINE_H