#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H
#include "pch.h"

namespace vlux {
class Swapchain {
   public:
    Swapchain() = delete;
    Swapchain(const VkPhysicalDevice physical_device, const VkDevice device,
              const VkSurfaceKHR surface, GLFWwindow* window, const bool vsync);
    ~Swapchain() {
        for (const auto& image_view : image_views_) {
            vkDestroyImageView(device_, image_view, nullptr);
        }
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }

    // acccessor
    VkSwapchainKHR GetVkSwapchain() const { return swapchain_; }
    const std::vector<VkImage>& GetVkImages() const { return images_; }
    VkFormat GetVkFormat() const {
        if (!format_.has_value()) {
            throw std::runtime_error("`Swapchain::format_` has no values.");
        }
        return format_.value();
    }
    const VkExtent2D& GetVkExtent() const {
        if (!extent_.has_value()) {
            throw std::runtime_error("`Swapchain::extent_` has no values.");
        }
        return extent_.value();
    }
    uint32_t GetWidth() const {
        if (!extent_.has_value()) {
            throw std::runtime_error("`Swapchain::extent_` has no values.");
        }
        return extent_->width;
    }
    uint32_t GetHeight() const {
        if (!extent_.has_value()) {
            throw std::runtime_error("`Swapchain::extent_` has no values.");
        }
        return extent_->height;
    }
    uint32_t GetImageCount() const {
        if (image_count_ == 0) {
            throw std::runtime_error("`Swapchain::image_count_` is zero.");
        }
        return image_count_;
    }
    const std::vector<VkImageView>& GetVkImageViews() const { return image_views_; }

   private:
    const VkDevice device_;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::optional<VkFormat> format_ = std::nullopt;
    std::optional<VkExtent2D> extent_ = std::nullopt;
    std::vector<VkImageView> image_views_;
    uint32_t image_count_;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes,
                                       const bool vsync);

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice physical_device,
                                              const VkSurfaceKHR surface);

}  // namespace vlux

#endif