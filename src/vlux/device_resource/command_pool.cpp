#include "command_pool.h"

namespace vlux {

CommandPool::CommandPool(const VkDevice device, const VkPhysicalDevice physical_device,
                         const VkSurfaceKHR surface)
    : device_(device) {
    const auto queue_family_indices = FindQueueFamilies(physical_device, surface);
    const auto pool_info = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_indices.graphics_family.value(),
    };
    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

CommandPool::~CommandPool() { vkDestroyCommandPool(device_, command_pool_, nullptr); }

}  // namespace vlux