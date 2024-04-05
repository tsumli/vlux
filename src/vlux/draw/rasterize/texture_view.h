#ifndef RASTERIZE_TEXTURE_VIEW_H
#define RASTERIZE_TEXTURE_VIEW_H
#include "pch.h"

namespace vlux::draw::rasterize {
struct TextureView {
    VkImageView color;
    VkImageView normal;
};
}  // namespace vlux::draw::rasterize

#endif