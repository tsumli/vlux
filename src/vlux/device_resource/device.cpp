#include "device.h"

#include <vulkan/vulkan_core.h>

#include "common/queue.h"
#include "swapchain.h"

namespace vlux {
namespace {
constexpr auto kDeviceExtensions = std::to_array<const char*>(
    {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
     VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
     VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, VK_KHR_SPIRV_1_4_EXTENSION_NAME,
     VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
     VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME});
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

    return IsQueueFamilyIndicesComplete(indices) && extensions_supported && swapchain_adequate &&
           supported_features.samplerAnisotropy;
}

Device::Device(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface) {
    const auto indices = FindQueueFamilies(physical_device, surface);
    std::set<uint32_t> unique_queue_families = {indices.graphics_compute_family.value(),
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

    auto scalar_block_layout_features = VkPhysicalDeviceScalarBlockLayoutFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
        .pNext = nullptr,
        .scalarBlockLayout = VK_TRUE,
    };

    auto syncronization_2_features = VkPhysicalDeviceSynchronization2Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &scalar_block_layout_features,
        .synchronization2 = VK_TRUE,
    };

    auto robustness_2_features = VkPhysicalDeviceRobustness2FeaturesEXT{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
        .pNext = &syncronization_2_features,
        .robustBufferAccess2 = VK_TRUE,
        .robustImageAccess2 = VK_TRUE,
        .nullDescriptor = VK_TRUE,
    };

    auto buffer_device_address_features = VkPhysicalDeviceBufferDeviceAddressFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &robustness_2_features,
        .bufferDeviceAddress = VK_TRUE,
        .bufferDeviceAddressCaptureReplay = VK_TRUE,
        .bufferDeviceAddressMultiDevice = VK_TRUE,
    };

    auto raytracing_pipeline_features = VkPhysicalDeviceRayTracingPipelineFeaturesKHR{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = &buffer_device_address_features,
        .rayTracingPipeline = VK_TRUE,
        .rayTracingPipelineTraceRaysIndirect = VK_TRUE,
        .rayTraversalPrimitiveCulling = VK_TRUE,
    };

    auto acceleration_structure_features = VkPhysicalDeviceAccelerationStructureFeaturesKHR{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = &raytracing_pipeline_features,
        .accelerationStructure = VK_TRUE,
        .accelerationStructureCaptureReplay = VK_TRUE,
        .descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE,
    };

    auto device_features = VkPhysicalDeviceFeatures2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &acceleration_structure_features,
        .features =
            VkPhysicalDeviceFeatures{
                .robustBufferAccess = VK_TRUE,
                .independentBlend = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
                .shaderInt64 = VK_TRUE,
            },
    };

    const auto create_info = VkDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_features,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size()),
        .ppEnabledLayerNames = kValidationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data(),
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