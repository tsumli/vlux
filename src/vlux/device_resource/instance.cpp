#include "instance.h"

#include "debug_messenger.h"

namespace vlux {
std::vector<const char*> GetRequiredExtensions() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (!kValidationLayers.empty()) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    // extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    // extensions.push_back("VK_KHR_get_physical_device_properties2");
    return extensions;
}

Instance::Instance() {
    const auto app_info = VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };
    const auto debug_create_info = CreateDebugMessengerCreateInfo();
    const auto extensions = GetRequiredExtensions();

    const auto create_info = [&]() {
        auto create_info = VkInstanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext =
                kValidationLayers.empty()
                    ? nullptr
                    : static_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info),
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
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
