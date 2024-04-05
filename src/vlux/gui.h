#ifndef IMGUI_H
#define IMGUI_H
#include "pch.h"
//
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace vlux {
[[maybe_unused]] static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties,
                                                  const char* extension) {
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0) return true;
    return false;
}

struct GuiInput {
    const VkInstance instance;
    const VkDevice device;
    const VkPhysicalDevice physical_device;
    GLFWwindow* window;
    const uint32_t queue_family;
    const VkQueue queue;
    const VkPipelineCache pipeline_cache;
    const VkRenderPass render_pass;
    const uint32_t image_count;
};

class Gui {
   public:
    Gui() = delete;
    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;
    Gui(const Gui&&) = delete;
    Gui& operator=(const Gui&&) = delete;

    Gui(const GuiInput& input);
    ~Gui();
    void CreateWindow();
    void OnStart() const;
    void Render(const VkCommandBuffer command_buffer) const;

   private:
    static constexpr uint32_t kMinImageCount = 2;
    GLFWwindow* window_;
    const VkInstance instance_;
    const VkDevice device_;
    const VkPhysicalDevice physical_device_;
    const uint32_t queue_family_;

    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
};
}  // namespace vlux

#endif