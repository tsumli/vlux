#ifndef RASTERIZE_H
#define RASTERIZE_H

#include "pch.h"
//
#include "common/descriptor_pool.h"
#include "common/descriptor_set_layout.h"
#include "common/descriptor_sets.h"
#include "common/frame_buffer.h"
#include "common/graphics_pipeline.h"
#include "common/pipeline_layout.h"
#include "common/render_pass.h"
#include "draw/draw_strategy.h"
#include "texture/texture_sampler.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {

class DrawRasterize final : public DrawStrategy {
   public:
    DrawRasterize(const UniformBuffer<TransformParams>& transform_ubo, Scene& scene,
                  const DeviceResource& device_resource);
    ~DrawRasterize() override = default;

    void RecordCommandBuffer(const Scene& scene, const uint32_t image_idx, const uint32_t cur_frame,
                             const VkExtent2D& swapchain_extent,
                             const VkCommandBuffer command_buffer) override;

    void OnRecreateSwapChain(const DeviceResource& device_resource) override;

    VkRenderPass GetVkRenderPass() const override { return render_pass_->GetVkRenderPass(); }
    VkDescriptorPool GetVkDescriptorPool() const override {
        return descriptor_pool_->GetVkDescriptorPool();
    }
    VkDescriptorSetLayout GetVkDescriptorSetLayout() const override {
        return descriptor_set_layout_->GetVkDescriptorSetLayout();
    }
    VkFramebuffer GetVkFrameBuffer(const size_t idx) const override {
        return framebuffer_.at(idx).GetVkFrameBuffer();
    }
    VkPipeline GetVkGraphicsPipeline() const override {
        return graphics_pipeline_->GetVkGraphicsPipeline();
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
};
}  // namespace vlux::draw::rasterize

#endif