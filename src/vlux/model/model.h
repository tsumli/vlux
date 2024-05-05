#ifndef MODEL_H
#define MODEL_H
#include <vulkan/vulkan_core.h>

#include <memory>

#include "common/image.h"
#include "pch.h"
//
#include "index.h"
#include "uniform_buffer.h"
#include "vertex.h"
//
#include "../texture/texture.h"

namespace vlux {

struct MaterialParams {
    alignas(16) glm::vec4 base_color_factor;
    alignas(16) glm::vec4 metallic_roughnes_factor;
};

class Model {
   public:
    using ModelPixelType = uint8_t;

    Model() = delete;
    Model(const VkDevice device, const VkPhysicalDevice physical_device,
          std::vector<VertexBuffer>&& vertex_buffer, std::vector<IndexBuffer>&& index_buffer,
          glm::vec4&& base_color_factor, const float metallic_factor, const float roughtness_factor,
          std::shared_ptr<Texture<ModelPixelType>>&& base_color_texture,
          std::shared_ptr<Texture<ModelPixelType>>&& normal_texture,
          std::shared_ptr<Texture<ModelPixelType>>&& emissive_texture,
          std::shared_ptr<Texture<ModelPixelType>>&& metallic_roughness_texture)
        : vertex_buffers_(std::move(vertex_buffer)),
          index_buffers_(std::move(index_buffer)),
          material_ubo_(std::make_unique<UniformBuffer<MaterialParams>>(device, physical_device)),
          base_color_texture_(std::move(base_color_texture)),
          normal_texture_(std::move(normal_texture)),
          emissive_texture_(std::move(emissive_texture)),
          metallic_roughness_texture_(std::move(metallic_roughness_texture)) {
        // update ubo
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            material_ubo_->UpdateUniformBuffer(
                {
                    .base_color_factor = base_color_factor,
                    .metallic_roughnes_factor = glm::vec4(metallic_factor, roughtness_factor, 0, 0),
                },
                frame_i);
        }
    }
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    const std::vector<VertexBuffer>& GetVertexBuffers() const { return vertex_buffers_; }
    const std::vector<IndexBuffer>& GetIndexBuffers() const { return index_buffers_; }

    const UniformBuffer<MaterialParams>& GetMaterialUbo() const { return *material_ubo_; }

    std::shared_ptr<Texture<ModelPixelType>> GetBaseColorTexture() const {
        return base_color_texture_;
    }
    std::shared_ptr<Texture<ModelPixelType>> GetNormalTexture() const { return normal_texture_; }
    std::shared_ptr<Texture<ModelPixelType>> GetEmissiveTexture() const {
        return emissive_texture_;
    }
    std::shared_ptr<Texture<ModelPixelType>> GetMetallicRoughnessTexture() const {
        return metallic_roughness_texture_;
    }

   private:
    std::vector<VertexBuffer> vertex_buffers_;
    std::vector<IndexBuffer> index_buffers_;

    std::unique_ptr<UniformBuffer<MaterialParams>> material_ubo_;

    std::shared_ptr<Texture<ModelPixelType>> base_color_texture_{nullptr};
    std::shared_ptr<Texture<ModelPixelType>> normal_texture_{nullptr};
    std::shared_ptr<Texture<ModelPixelType>> emissive_texture_{nullptr};
    std::shared_ptr<Texture<ModelPixelType>> metallic_roughness_texture_{nullptr};
    // std::optional<Texture> occlusion_texture_ = std::nullopt;
};

}  // namespace vlux

#endif