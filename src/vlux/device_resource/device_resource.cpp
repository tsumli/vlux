#include "device_resource.h"

#include "common/queue.h"

namespace vlux {
DeviceResource::DeviceResource(const Window& window, const bool vsync) : window_(window) {
    spdlog::debug("Creating Vulkan instance");
    instance_.emplace();
    spdlog::debug("Creating debug messenger");
    debug_messenger_.emplace(instance_->GetVkInstance());
    spdlog::debug("Creating surface");
    surface_.emplace(instance_->GetVkInstance(), window.GetGLFWwindow());
    spdlog::debug("Picking physical device");
    physical_device_ = PickPhysicalDevice(instance_->GetVkInstance(), surface_->GetVkSurface());
    spdlog::debug("Creating logical device");
    device_.emplace(physical_device_, surface_->GetVkSurface());
    spdlog::debug("Creating queues");
    queues_ = CreateQueue(device_->GetVkDevice(), physical_device_, surface_->GetVkSurface());
    spdlog::debug("Creating swapchain");
    swapchain_.emplace(physical_device_, device_->GetVkDevice(), surface_->GetVkSurface(),
                       window_.GetGLFWwindow(), vsync);
    spdlog::debug("Creating sync object");
    sync_object_.emplace(device_->GetVkDevice());
}

DeviceResource::~DeviceResource() {}

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
}  // namespace vlux