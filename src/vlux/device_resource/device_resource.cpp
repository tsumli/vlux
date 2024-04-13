#include "device_resource.h"

#include "common/queue.h"
#include "swapchain.h"

namespace vlux {
DeviceResource::DeviceResource(const Window& window) : window_(window) {
    instance_.emplace();
    debug_messenger_.emplace(instance_->GetVkInstance());
    surface_.emplace(instance_->GetVkInstance(), window.GetGLFWwindow());
    physical_device_ = PickPhysicalDevice(instance_->GetVkInstance(), surface_->GetVkSurface());
    device_.emplace(physical_device_, surface_->GetVkSurface());
    std::tie(graphics_queue_, present_queue_) =
        CreateQueue(device_->GetVkDevice(), physical_device_, surface_->GetVkSurface());
    swapchain_.emplace(physical_device_, device_->GetVkDevice(), surface_->GetVkSurface(),
                       window_.GetGLFWwindow());
    const auto [width, height] = window.GetWindowSize();
    depth_stencil_.emplace(width, height, device_->GetVkDevice(), physical_device_);
    command_pool_.emplace(device_->GetVkDevice(), physical_device_, surface_->GetVkSurface());
    command_buffer_.emplace(command_pool_->GetVkCommandPool(), device_->GetVkDevice());
    sync_object_.emplace(device_->GetVkDevice());
}

DeviceResource::~DeviceResource() {}
}  // namespace vlux