#ifndef DRAW_RAYTRACING_H
#define DRAW_RAYTRACING_H

#include "pch.h"
//
#include "acceleration_structure.h"
#include "camera.h"
#include "common/descriptor_pool.h"
#include "common/descriptor_set_layout.h"
#include "common/descriptor_sets.h"
#include "common/image.h"
#include "common/pipeline_layout.h"
#include "common/raytracing_pipeline.h"
#include "draw/draw_strategy.h"
#include "light.h"
#include "scene/scene.h"
#include "texture/texture_sampler.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::raytracing {
// Holds data for a ray tracing scratch buffer that is used as a temporary storage
struct ModePushConstants {
    uint32_t mode;
};
class DrawRaytracing : public DrawStrategy {
   public:
    DrawRaytracing(const UniformBuffer<TransformParams>& transform_ubo,
                   const UniformBuffer<CameraParams>& camera_ubo,
                   const UniformBuffer<CameraMatrixParams>& camera_matrix_ubo,
                   const UniformBuffer<LightParams>& light_ubo, Scene& scene, const VkQueue queue,
                   const VkCommandPool command_pool, const DeviceResource& device_resource);
    ~DrawRaytracing() override = default;
    DrawRaytracing(const DrawRaytracing&) = delete;
    DrawRaytracing& operator=(const DrawRaytracing&) = delete;
    DrawRaytracing(DrawRaytracing&&) = default;
    DrawRaytracing& operator=(DrawRaytracing&&) = default;

    void RecordCommandBuffer(const uint32_t image_idx, const VkExtent2D& swapchain_extent,
                             const VkCommandBuffer command_buffer) override;

    void OnRecreateSwapChain(const DeviceResource& device_resource) override;

    const ImageBuffer& GetOutputRenderTarget() const override {
        return render_targets_.at(RenderTargetType::kFinalized).value();
    }

    void SetMode(const uint32_t mode) override { mode_ = mode; };
    uint32_t GetMode() const override { return mode_; };

   private:
    void CreateBottomLevelAS(const VkDevice device, const VkPhysicalDevice physical_device,
                             const VkQueue queue, const VkCommandPool command_pool);

    void CreateTopLevelAS(const VkDevice device, const VkPhysicalDevice physical_device,
                          const VkQueue queue, const VkCommandPool command_pool);

    void CreateShaderBindingTable(const VkDevice device, const VkPhysicalDevice physical_device,
                                  const VkPipeline pipeline);

    uint64_t GetBufferDeviceAddress(const VkDevice device, const VkBuffer buffer);

    // scene
    const Scene& scene_;

    // device resource
    const VkDevice device_;

    // render targets
    enum class RenderTargetType { kFinalized, kCount };
    std::unordered_map<RenderTargetType, std::optional<ImageBuffer>> render_targets_;

    // Ray tracing acceleration structure
    std::vector<AccelerationStructure> bottom_level_as_;
    std::optional<AccelerationStructure> top_level_as_;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups_{};

    uint32_t mode_{0};

    std::optional<Buffer> vertex_buffer_;
    std::optional<Buffer> index_buffer_;
    std::optional<Buffer> transform_buffer_;
    std::optional<Buffer> raygen_shader_binding_table_;
    std::optional<Buffer> miss_shader_binding_table_;
    std::optional<Buffer> hit_shader_binding_table_;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_pipeline_properties_{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features_{};

    // Rendering objects
    std::optional<DescriptorPool> raytracing_descriptor_pool_;
    //! (kMaxFramesInFlight,)
    std::vector<DescriptorSets> raytracing_descriptor_sets_;
    //! (kNumDescriptorSetRaytracing,)
    std::vector<DescriptorSetLayout> raytracing_descriptor_set_layout_;
    //! (kMaxFramesInFlight,)
    std::vector<PipelineLayout> raytracing_pipeline_layout_;
    //! (kMaxFramesInFlight,)
    std::vector<RaytracingPipeline> raytracing_pipeline_;

    enum class TextureSamplerType { kColor, kCount };
    std::unordered_map<TextureSamplerType, std::optional<TextureSampler>> texture_samplers_;

    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
};
}  // namespace vlux::draw::raytracing

#endif