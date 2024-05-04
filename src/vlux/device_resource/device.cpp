#include "device.h"

#include "swapchain.h"

namespace vlux {
namespace {
constexpr auto kDeviceExtensions = std::to_array<const char*>({
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
});
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice physical_device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

    auto available_extensions = std::vector<VkExtensionProperties>(extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                         available_extensions.data());

    auto required_extensions =
        std::set<std::string>(kDeviceExtensions.begin(), kDeviceExtensions.end());
    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

bool IsDeviceSuitable(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface) {
    const auto indices = FindQueueFamilies(physical_device, surface);
    auto extensions_supported = CheckDeviceExtensionSupport(physical_device);

    bool swapchain_adequate = false;
    if (extensions_supported) {
        auto swapchain_support = QuerySwapChainSupport(physical_device, surface);
        swapchain_adequate =
            !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
    }

    auto supported_features = VkPhysicalDeviceFeatures{};
    vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

    return indices.IsComplete() && extensions_supported && swapchain_adequate &&
           supported_features.samplerAnisotropy;
}

Device::Device(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface) {
    const auto indices = FindQueueFamilies(physical_device, surface);
    std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(),
                                                indices.present_family.value()};

    const float queue_priority = 1.0f;
    const auto queue_create_infos = [&]() {
        auto queue_create_infos = std::vector<VkDeviceQueueCreateInfo>();
        for (const auto queue_family : unique_queue_families) {
            const auto queue_create_info = VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queue_family,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            };
            queue_create_infos.push_back(queue_create_info);
        }
        return queue_create_infos;
    }();

    constexpr auto kDeviceFeatures = VkPhysicalDeviceFeatures{
        .independentBlend = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    const auto create_info = VkDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size()),
        .ppEnabledLayerNames = kValidationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data(),
        .pEnabledFeatures = &kDeviceFeatures,
    };

    if (vkCreateDevice(physical_device, &create_info, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
}

VkPhysicalDevice PickPhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface) {
    auto device_count = [&]() {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        return device_count;
    }();

    if (device_count == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    const auto devices = [&]() {
        auto devices = std::vector<VkPhysicalDevice>(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
        return devices;
    }();

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device, surface)) {
            physical_device = device;
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    return physical_device;
}

}  // namespace vlux