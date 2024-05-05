#ifndef GLTF_H
#define GLTF_H
#include "pch.h"
//
#include "index.h"
#include "vertex.h"
//
#include "../texture/texture.h"

namespace vlux {
tinygltf::Model LoadTinyGltfModel(const std::filesystem::path& path);

struct GltfObject {
    std::vector<Index> indices;
    std::vector<Vertex> vertices;
    glm::vec4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
    std::shared_ptr<Texture> base_color_texture;
    std::shared_ptr<Texture> normal_texture;
    std::shared_ptr<Texture> occlusion_texture;
    std::shared_ptr<Texture> emissive_texture;
    std::shared_ptr<Texture> metallic_roughness_texture;
};

/**
 * @brief
 *
 * @param primitive
 * @param model
 * @param graphics_queue
 * @param command_pool
 * @param physical_device
 * @param device
 * @param scale
 * @param translation
 * @param rotation vec3 (yaw, pitch, roll). It is in degrees.
 * @return GltfObject
 */
GltfObject LoadGltfObjects(const tinygltf::Primitive& primitive, const tinygltf::Model& model,
                           const VkQueue graphics_queue, const VkCommandPool command_pool,
                           const VkPhysicalDevice physical_device, const VkDevice device,
                           const float scale, const glm::vec3& translation = {0.0f, 0.0f, 0.0f},
                           const glm::vec3& rotation = {0.0f, 0.0f, 0.0f});

class GLTF {
   public:
    GLTF() = delete;
    GLTF(const std::filesystem::path& path) : path_(path) {}
    GLTF(const GLTF&) = delete;
    GLTF& operator=(const GLTF&) = delete;
    GLTF(GLTF&&) = default;
    GLTF& operator=(GLTF&&) = default;

   private:
    std::filesystem::path path_;
};

}  // namespace vlux

#endif