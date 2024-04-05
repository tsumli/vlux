#ifndef MODEL_H
#define MODEL_H
#include <memory>

#include "pch.h"
//
#include "index.h"
#include "vertex.h"
//
#include "../texture/texture.h"

namespace vlux {

class Model {
   public:
    Model() = default;
    Model(std::vector<VertexBuffer>&& vertex_buffer, std::vector<IndexBuffer>&& index_buffer,
          std::shared_ptr<Texture>&& base_color_texture, std::shared_ptr<Texture>&& normal_texture)
        : vertex_buffers_(std::move(vertex_buffer)),
          index_buffers_(std::move(index_buffer)),
          base_color_texture_(std::move(base_color_texture)),
          normal_texture_(std::move(normal_texture)) {}
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    const std::vector<VertexBuffer>& GetVertexBuffers() const { return vertex_buffers_; }
    const std::vector<IndexBuffer>& GetIndexBuffers() const { return index_buffers_; }
    const std::shared_ptr<Texture> GetBaseColorTexture() const { return base_color_texture_; }
    const std::shared_ptr<Texture> GetNormalTexture() const { return normal_texture_; }
    // const Texture& GetOcclusionTexture() const {
    //     if (!occlusion_texture_.has_value())
    //         throw std::runtime_error("occlusion texture is not set");
    //     return occlusion_texture_.value();
    // }
    // const Texture& GetEmissiveTexture() const {
    //     if (!emissive_texture_.has_value()) throw std::runtime_error("emissive texture is not
    //     set"); return emissive_texture_.value();
    // }

   private:
    std::vector<VertexBuffer> vertex_buffers_;
    std::vector<IndexBuffer> index_buffers_;

    std::shared_ptr<Texture> base_color_texture_ = nullptr;
    std::shared_ptr<Texture> normal_texture_ = nullptr;
    // std::optional<Texture> occlusion_texture_ = std::nullopt;
    // std::optional<Texture> emissive_texture_ = std::nullopt;
};

}  // namespace vlux

#endif