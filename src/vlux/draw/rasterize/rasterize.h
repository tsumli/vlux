#ifndef RASTERIZE_H
#define RASTERIZE_H

#include "pch.h"
//
#include "device_resource/device_resource.h"
#include "draw/draw_strategy.h"
#include "draw/rasterize/descriptor_pool.h"
#include "draw/rasterize/descriptor_set.h"
#include "draw/rasterize/descriptor_set_layout.h"
#include "draw/rasterize/frame_buffer.h"
#include "draw/rasterize/graphics_pipeline.h"
#include "draw/rasterize/render_pass.h"
#include "scene/scene.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {

class DrawRasterize final : public DrawStrategy {
   public:
    DrawRasterize(const UniformBuffer<TransformParams>& transform_ubo, Scene& scene,
                  const DeviceResource& device_resource);
    ~DrawRasterize() override = default;

    void RecordCommandBuffer(const Scene& scene, const uint32_t image_idx, const uint32_t cur_frame,
                             const std::vector<VkFramebuffer>& swapchain_framebuffers,
                             const VkExtent2D& swapchain_extent, const VkPipeline graphics_pipeline,
                             const VkCommandBuffer command_buffer) override;

    void OnRecreateSwapChain(const DeviceResource& device_resource) override;

    VkRenderPass GetVkRenderPass() const override { return render_pass_.GetVkRenderPass(); }
    VkDescriptorPool GetVkDescriptorPool() const override {
        return descriptor_pool_.GetVkDescriptorPool();
    }
    VkDescriptorSetLayout GetVkDescriptorSetLayout() const override {
        return descriptor_set_layout_.GetVkDescriptorSetLayout();
    }
    const std::vector<VkFramebuffer>& GetVkFrameBuffers() const override {
        return framebuffer_.GetVkFrameBuffers();
    }
    VkPipeline GetVkGraphicsPipeline() const override {
        return graphics_pipeline_.GetVkGraphicsPipeline();
    }

   private:
    const Scene& scene_;
    const UniformBuffer<TransformParams>& transform_ubo_;

    RenderPass render_pass_;
    DescriptorSetLayout descriptor_set_layout_;
    DescriptorPool descriptor_pool_;
    GraphicsPipeline graphics_pipeline_;
    FrameBuffer framebuffer_;
    std::vector<DescriptorSet> descriptor_sets_;
};
}  // namespace vlux::draw::rasterize

#endif