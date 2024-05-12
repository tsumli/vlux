#ifndef DRAW_RASTERIZE_H
#define DRAW_RASTERIZE_H

#include "pch.h"
//
#include "camera.h"
#include "common/compute_pipeline.h"
#include "common/descriptor_pool.h"
#include "common/descriptor_set_layout.h"
#include "common/descriptor_sets.h"
#include "common/frame_buffer.h"
#include "common/graphics_pipeline.h"
#include "common/image.h"
#include "common/pipeline_layout.h"
#include "common/render_pass.h"
#include "draw/draw_strategy.h"
#include "light.h"
#include "scene/scene.h"
#include "texture/texture_sampler.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {

struct ModePushConstants {
    uint32_t mode;
};

class DrawRasterize final : public DrawStrategy {
   public:
    DrawRasterize(const UniformBuffer<TransformParams>& transform_ubo,
                  const UniformBuffer<CameraParams>& camera_ubo,
                  const UniformBuffer<LightParams>& light_ubo, Scene& scene,
                  const DeviceResource& device_resource);
    ~DrawRasterize() override = default;

    void RecordCommandBuffer(const uint32_t image_idx, const VkExtent2D& swapchain_extent,
                             const VkCommandBuffer command_buffer) override;

    void OnRecreateSwapChain(const DeviceResource& device_resource) override;
    const ImageBuffer& GetOutputRenderTarget() const override {
        return render_targets_.at(RenderTargetType::kFinalized).value();
    }

    void SetMode(const uint32_t mode) override { mode_ = mode; }
    uint32_t GetMode() const override { return mode_; }

   private:
    const Scene& scene_;

    std::optional<RenderPass> render_pass_;

    std::optional<DescriptorPool> graphics_descriptor_pool_;
    //! (kMaxFramesInFlight,)
    std::vector<DescriptorSets> graphics_descriptor_sets_;
    //! (kNumDescriptorSetGraphics,)
    std::vector<DescriptorSetLayout> graphics_descriptor_set_layout_;
    //! (kMaxFramesInFlight,)
    std::vector<PipelineLayout> graphics_pipeline_layout_;
    //! (kMaxFramesInFlight,)
    std::vector<GraphicsPipeline> graphics_pipeline_;

    std::optional<DescriptorPool> compute_descriptor_pool_;
    //! (kMaxFramesInFlight,)
    std::vector<DescriptorSets> compute_descriptor_sets_;
    //! (kNumDescriptorSetCompute,)
    std::vector<DescriptorSetLayout> compute_descriptor_set_layout_;
    //! (kMaxFramesInFlight,)
    std::vector<PipelineLayout> compute_pipeline_layout_;
    //! (kMaxFramesInFlight,)
    std::vector<ComputePipeline> compute_pipeline_;
    //! (kMaxFramesInFlight,)
    std::vector<FrameBuffer> framebuffer_;

    // render targets
    enum class RenderTargetType {
        kColor,
        kNormal,
        kDepthStencil,
        kPosition,
        kEmissive,
        kBaseColorFactor,
        kMetallicRoughnessFactor,
        kMetallicRoughness,
        kFinalized,
        kCount
    };
    std::unordered_map<RenderTargetType, std::optional<ImageBuffer>> render_targets_;

    enum class TextureSamplerType {
        kColor,
        kNormal,
        kEmissive,
        kOcclusionRoughnessMetallic,
        kCount
    };
    std::unordered_map<TextureSamplerType, std::optional<TextureSampler>> texture_samplers_;

    // mode
    uint32_t mode_{0};
};
}  // namespace vlux::draw::rasterize

#endif