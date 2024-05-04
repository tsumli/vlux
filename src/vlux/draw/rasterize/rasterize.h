#ifndef DRAW_RASTERIZE_H
#define DRAW_RASTERIZE_H

#include "light.h"
#include "pch.h"
//
#include "camera.h"
#include "common/compute_pipeline.h"
#include "common/descriptor_pool.h"
#include "common/descriptor_set_layout.h"
#include "common/descriptor_sets.h"
#include "common/frame_buffer.h"
#include "common/graphics_pipeline.h"
#include "common/pipeline_layout.h"
#include "common/render_pass.h"
#include "common/render_target.h"
#include "draw/draw_strategy.h"
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

    VkRenderPass GetRenderPass() const override { return render_pass_.value().GetVkRenderPass(); }
    VkFramebuffer GetFramebuffer(const size_t idx) const override {
        return framebuffer_.at(idx).GetVkFrameBuffer();
    }
    void RecordCommandBuffer(const uint32_t image_idx, const VkExtent2D& swapchain_extent,
                             const VkCommandBuffer command_buffer) override;

    void OnRecreateSwapChain(const DeviceResource& device_resource) override;
    const RenderTarget& GetOutputRenderTarget() const override {
        return render_targets_.at(RenderTargetType::kFinalized).value();
    }

    void SetMode(const uint32_t mode) override { mode_ = mode; }
    uint32_t GetMode() const override { return mode_; }

   private:
    const Scene& scene_;

    std::optional<RenderPass> render_pass_;

    std::optional<DescriptorPool> graphics_descriptor_pool_;
    std::vector<DescriptorSets> graphics_descriptor_sets_;
    std::optional<DescriptorSetLayout> graphics_descriptor_set_layout_;
    std::optional<PipelineLayout> graphics_pipeline_layout_;
    std::optional<GraphicsPipeline> graphics_pipeline_;

    std::optional<DescriptorPool> compute_descriptor_pool_;
    std::vector<DescriptorSets> compute_descriptor_sets_;
    std::vector<DescriptorSetLayout> compute_descriptor_set_layout_;
    std::vector<PipelineLayout> compute_pipeline_layout_;
    std::vector<ComputePipeline> compute_pipeline_;

    std::vector<FrameBuffer> framebuffer_;

    // render targets
    enum class RenderTargetType {
        kColor,
        kNormal,
        kDepthStencil,
        kPosition,
        kEmissive,
        kFinalized,
        kCount
    };
    std::unordered_map<RenderTargetType, std::optional<RenderTarget>> render_targets_;

    enum class TextureSamplerType { kColor, kNormal, kEmissive, kCount };
    std::unordered_map<TextureSamplerType, std::optional<TextureSampler>> texture_samplers_;

    // mode
    uint32_t mode_{0};
};
}  // namespace vlux::draw::rasterize

#endif