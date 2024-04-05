#ifndef SURFACE_H
#define SURFACE_H
#include "pch.h"

namespace vlux {
VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& available_formats);

class Surface {
   public:
    Surface(const VkInstance instance, GLFWwindow* window) : instance_(instance) {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    };
    ~Surface() { vkDestroySurfaceKHR(instance_, surface_, nullptr); }
    VkSurfaceKHR GetVkSurface() const { return surface_; }

   private:
    const VkInstance instance_;
    VkSurfaceKHR surface_;
};

}  // namespace vlux
#endif