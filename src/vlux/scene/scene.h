#ifndef SCENE_H
#define SCENE_H
#include "pch.h"
//
#include "cubemap/cubemap.h"
#include "model/model.h"

namespace vlux {

class Scene {
   public:
    Scene(std::vector<Model>&& models, std::optional<CubeMap>&& cubemap = std::nullopt)
        : models_(std::move(models)), cubemap_(std::move(cubemap)){};
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = delete;

    const std::vector<Model>& GetModels() const { return models_; }
    const std::optional<CubeMap>& GetCubemap() const { return cubemap_; }

   private:
    std::vector<Model> models_;
    std::optional<CubeMap> cubemap_{std::nullopt};
};

}  // namespace vlux

#endif