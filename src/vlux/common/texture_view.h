#ifndef COMMON_TEXTURE_VIEW_H
#define COMMON_TEXTURE_VIEW_H
#include "pch.h"

namespace vlux {
struct TextureView {
    VkImageView color;
    VkImageView normal;
};
}  // namespace vlux

#endif