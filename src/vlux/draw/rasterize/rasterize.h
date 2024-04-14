#ifndef DRAW_RASTERIZE_H
#define DRAW_RASTERIZE_H

#include <vulkan/vulkan_core.h>

#include <unordered_map>

#include "pch.h"
//
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

class DrawRasterize final : public DrawStrategy {
   public:
    DrawRasterize(const UniformBuffer<TransformParams>& transform_ubo, Scene& scene,
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
        return render_targets_.at(RenderTargetType::kColor).value();
    }

   private:
    const Scene& scene_;
    const UniformBuffer<TransformParams>& transform_ubo_;

    std::optional<RenderPass> render_pass_;
    std::optional<DescriptorSetLayout> descriptor_set_layout_;
    std::optional<DescriptorPool> descriptor_pool_;
    std::optional<PipelineLayout> pipeline_layout_;
    std::optional<GraphicsPipeline> graphics_pipeline_;
    std::vector<FrameBuffer> framebuffer_;
    std::vector<std::vector<DescriptorSets>> descriptor_sets_;
    std::shared_ptr<TextureSampler> texture_sampler_;

    // render targets
    enum class RenderTargetType { kColor, kNormal, kDepthStencil, kCount };
    std::unordered_map<RenderTargetType, std::optional<RenderTarget>> render_targets_;
};
}  // namespace vlux::draw::rasterize

#endif