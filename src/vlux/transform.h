#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "pch.h"

namespace vlux {
struct TransformParams {
    alignas(16) glm::mat4x4 world;
    alignas(16) glm::mat4x4 view_proj;
    alignas(16) glm::mat4x4 world_view_proj;
    alignas(16) glm::mat4x4 proj_to_world;
};
}  // namespace vlux

#endif