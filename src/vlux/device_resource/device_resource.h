#ifndef DEVICE_RESOURCE_H
#define DEVICE_RESOURCE_H
#include "pch.h"
//
#include "command_buffer.h"
#include "command_pool.h"
#include "debug_messenger.h"
#include "device.h"
#include "instance.h"
#include "surface.h"
#include "swapchain.h"
#include "sync_object.h"
#include "window.h"

namespace vlux {

class DeviceResource {
   public:
    DeviceResource(const Window& window);
    ~DeviceResource();  // TODO: destructor order

    // accessor
    GLFWwindow* GetGLFWwindow() const { return window_.GetGLFWwindow(); }
    const Window& GetWindow() const { return window_; }
    const Device& GetDevice() const {
        if (!device_.has_value()) {
            throw std::runtime_error("`DeviceResource::device_` has no values.");
        }
        return device_.value();
    }
    const Surface& GetSurface() const {
        if (!surface_.has_value()) {
            throw std::runtime_error("`DeviceResource::surface_` has no values.");
        }
        return surface_.value();
    }
    VkPhysicalDevice GetVkPhysicalDevice() const {
        if (physical_device_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::physical_device_ == `VK_NULL_HANDLE`");
        }
        return physical_device_;
    }
    const Instance& GetInstance() const {
        if (!instance_.has_value()) {
            throw std::runtime_error("`DeviceResource::instance_ has no values.");
        }
        return instance_.value();
    }
    const SyncObject& GetSyncObject() const {
        if (!sync_object_.has_value()) {
            throw std::runtime_error("`DeviceResource::sync_object_` has no values.");
        }
        return sync_object_.value();
    }
    const Swapchain& GetSwapchain() const {
        if (!swapchain_.has_value()) {
            throw std::runtime_error("`DeviceResource::swapchain_` has no values.");
        }
        return swapchain_.value();
    }
    const CommandBuffer& GetCommandBuffer() const {
        if (!command_buffer_.has_value()) {
            throw std::runtime_error("`DeviceResource::command_buffer_` has no values.");
        }
        return command_buffer_.value();
    }
    const CommandPool& GetCommandPool() const {
        if (!command_pool_.has_value()) {
            throw std::runtime_error("`DeviceResource::command_pool_` has no values.");
        }
        return command_pool_.value();
    }
    const VkQueue& GetGraphicsQueue() const {
        if (graphics_queue_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::graphics_queue_ == `VK_NULL_HANDLE`");
        }
        return graphics_queue_;
    }
    const VkQueue& GetPresentQueue() const {
        if (present_queue_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`DeviceResource::present_queue_ == `VK_NULL_HANDLE`");
        }
        return present_queue_;
    }

    // method
    void DeviceWaitIdle() const { vkDeviceWaitIdle(device_->GetVkDevice()); }

    void RecreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window_.GetGLFWwindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window_.GetGLFWwindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device_->GetVkDevice());

        swapchain_.reset();

        swapchain_.emplace(physical_device_, device_->GetVkDevice(), surface_->GetVkSurface(),
                           window_.GetGLFWwindow());
    }

   private:
    const Window window_;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;

    std::optional<Instance> instance_ = std::nullopt;
    std::optional<Device> device_ = std::nullopt;
    std::optional<Surface> surface_ = std::nullopt;
    std::optional<Swapchain> swapchain_ = std::nullopt;
    std::optional<DebugMessenger> debug_messenger_ = std::nullopt;
    std::optional<CommandPool> command_pool_ = std::nullopt;
    std::optional<CommandBuffer> command_buffer_ = std::nullopt;
    std::optional<SyncObject> sync_object_ = std::nullopt;
};
}  // namespace vlux

#endif