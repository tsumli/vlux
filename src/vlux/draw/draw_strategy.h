#ifndef DRAW_STRATEGY_H
#define DRAW_STRATEGY_H

#include "common/image.h"
#include "device_resource/device_resource.h"
namespace vlux::draw {
class DrawStrategy {
   public:
    virtual ~DrawStrategy() = default;

    virtual void RecordCommandBuffer(const uint32_t image_idx, const VkExtent2D& swapchain_extent,
                                     const VkCommandBuffer command_buffer) = 0;
    virtual void OnRecreateSwapChain(const DeviceResource& device_resource) = 0;
    virtual const ImageBuffer& GetOutputRenderTarget() const = 0;

    virtual void SetMode(const uint32_t mode) = 0;
    virtual uint32_t GetMode() const = 0;
};
}  // namespace vlux::draw

#endif