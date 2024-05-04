#ifndef LIGHT_H
#define LIGHT_H

#include "pch.h"

namespace vlux {
struct LightParams {
    alignas(16) glm::vec4 pos;
    alignas(4) float range;
    alignas(16) glm::vec4 color;
};
}  // namespace vlux

#endif  // LIGHT_H