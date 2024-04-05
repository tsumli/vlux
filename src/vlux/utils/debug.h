#ifndef UTILS_DEBUG_H
#define UTILS_DEBUG_H
#include "../pch.h"

namespace vlux {
void inline ThrowIfFailed(const VkResult result, const std::string_view msg) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(msg.data());
    }
}
}  // namespace vlux

#endif
