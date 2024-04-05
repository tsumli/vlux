#ifndef SCENE_H
#define SCENE_H
#include "pch.h"
//
#include "model/model.h"

namespace vlux {

class Scene {
   public:
    Scene(std::vector<Model>&& models) : models_(std::move(models)){};
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = delete;

    const std::vector<Model>& GetModels() const { return models_; }

   private:
    std::vector<Model> models_;
};

}  // namespace vlux

#endif