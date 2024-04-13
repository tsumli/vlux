#ifndef DRAW_STRATEGY_H
#define DRAW_STRATEGY_H

#include "device_resource/device_resource.h"
namespace vlux::draw {
class DrawStrategy {
   public:
    virtual ~DrawStrategy() = default;

    // accessor
    virtual VkRenderPass GetVkRenderPass() const = 0;
    virtual VkDescriptorPool GetVkDescriptorPool() const = 0;
    virtual VkDescriptorSetLayout GetVkDescriptorSetLayout() const = 0;
    virtual VkFramebuffer GetVkFrameBuffer(const size_t idx) const = 0;

    virtual VkPipeline GetVkGraphicsPipeline() const = 0;

    virtual void RecordCommandBuffer(const Scene& scene, const uint32_t image_idx,
                                     const uint32_t cur_frame, const VkExtent2D& swapchain_extent,
                                     const VkCommandBuffer command_buffer) = 0;
    virtual void OnRecreateSwapChain(const DeviceResource& device_resource) = 0;
};
}  // namespace vlux::draw

#endif