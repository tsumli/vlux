#ifndef DEBUG_MESSENGER_H
#define DEBUG_MESSENGER_H
#include "pch.h"

namespace vlux {
inline static VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
              [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT message_type,
              [[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
              [[maybe_unused]] void* p_user_data) {
    std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;
    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();
bool CheckValidationLayerSupport();

class DebugMessenger {
   public:
    DebugMessenger() = delete;
    DebugMessenger(const VkInstance instance);
    ~DebugMessenger();

   private:
    const VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
};
}  // namespace vlux

#endif