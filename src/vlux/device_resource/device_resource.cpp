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
                       window_.GetGLFWwindow(), false);
    sync_object_.emplace(device_->GetVkDevice());
}

void DeviceResource::DeviceWaitIdle() const { vkDeviceWaitIdle(device_->GetVkDevice()); }

void DeviceResource::RecreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_.GetGLFWwindow(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window_.GetGLFWwindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device_->GetVkDevice());

    swapchain_.reset();

    swapchain_.emplace(physical_device_, device_->GetVkDevice(), surface_->GetVkSurface(),
                       window_.GetGLFWwindow(), false);
}
DeviceResource::~DeviceResource() {}
}  // namespace vlux