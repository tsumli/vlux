#ifndef DRAW_STRATEGY_H
#define DRAW_STRATEGY_H

#include "common/render_target.h"
#include "device_resource/device_resource.h"
namespace vlux::draw {
class DrawStrategy {
   public:
    virtual ~DrawStrategy() = default;

    virtual VkRenderPass GetVkRenderPass() const = 0;
    virtual void RecordCommandBuffer(const uint32_t image_idx, const uint32_t cur_frame,
                                     const VkExtent2D& swapchain_extent,
                                     const VkCommandBuffer command_buffer) = 0;
    virtual void OnRecreateSwapChain(const DeviceResource& device_resource) = 0;
    virtual const RenderTarget& GetOutputRenderTarget() const = 0;
};
}  // namespace vlux::draw

#endif