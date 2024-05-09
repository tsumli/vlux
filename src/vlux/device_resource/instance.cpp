#include "instance.h"

#include "debug_messenger.h"

namespace vlux {
namespace {
const auto kEnabledValidationFeatures = std::vector<VkValidationFeatureEnableEXT>{
    // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
};
}

std::vector<const char*> GetRequiredExtensions() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (!kValidationLayers.empty()) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        // extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }
    return extensions;
}

Instance::Instance() {
    const auto app_info = VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vlux",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName = "Vlux Engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    auto validaton_features = VkValidationFeaturesEXT{
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext = nullptr,
        .enabledValidationFeatureCount = static_cast<uint32_t>(kEnabledValidationFeatures.size()),
        .pEnabledValidationFeatures = kEnabledValidationFeatures.data(),
    };

    auto debug_create_info = CreateDebugMessengerCreateInfo();
    debug_create_info.pNext = &validaton_features;

    const auto extensions = GetRequiredExtensions();

    const auto create_info = [&]() {
        auto create_info = VkInstanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext =
                kValidationLayers.empty()
                    ? nullptr
                    : static_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info),
            .flags = 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size()),
            .ppEnabledLayerNames = kValidationLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };
        return create_info;
    }();
    if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance");
    }
}

}  // namespace vlux
