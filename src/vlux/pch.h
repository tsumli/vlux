#ifndef PCH_H
#define PCH_H

// stl
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <random>
#include <ranges>
#include <set>
#include <source_location>
#include <string>
#include <vector>

// vulkan
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

// spdlog
#include "spdlog/spdlog.h"

// fmt
#include "fmt/core.h"

// json
#include "nlohmann/json.hpp"

// tinygltf
#include <stb_image.h>
#include <tiny_gltf.h>

#ifdef NDEBUG
constexpr auto kValidationLayers = std::array<const char*, 0>();
#else
constexpr auto kValidationLayers = std::to_array<const char*>({"VK_LAYER_KHRONOS_validation"});
#endif

constexpr auto kMaxFramesInFlight = 2;

#endif
