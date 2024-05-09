#ifndef CAMERA_H
#define CAMERA_H
#include "pch.h"
//
#include "transform.h"

namespace vlux {
struct CameraParams {
    alignas(16) glm::vec4 position;
};

struct CameraMatrixParams {
    alignas(16) glm::mat4 view_inverse;
    alignas(16) glm::mat4 proj_inverse;
};

class Camera {
   public:
    Camera() = delete;
    Camera(const glm::vec3& pos, const glm::vec3& rot, const float width, const float height);
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = default;
    Camera& operator=(Camera&&) = default;

    const glm::vec3& GetPosition() const { return pos_; }
    const glm::vec3& GetRotation() const { return rot_; }
    void UpdatePosition(const int move_forward, const int move_right, const int move_up,
                        const float gain);
    void UpdateRotation(const float cur_right, const float cur_up, const float gain);

    glm::mat4x4 CreateViewMatrix() const;

    TransformParams CreateTransformParams();
    CameraMatrixParams CreateCameraMatrixParams();

   private:
    glm::vec3 pos_{};
    glm::vec3 rot_{};

    glm::mat4x4 world_matrix_;
    glm::mat4x4 proj_matrix_;

    constexpr static glm::vec4 kLookat = {0.0f, 0.0f, 1.0f, 0.0f};
    constexpr static glm::vec4 kCamRight = {1.0f, 0.0f, 0.0f, 0.0f};
    constexpr static glm::vec4 kCamUp = {0.0f, 1.0f, 0.0f, 0.0f};

    glm::vec4 lookat_ = kLookat;
    glm::vec4 cam_right_ = kCamRight;
    glm::vec4 cam_up_ = kCamUp;
};
}  // namespace vlux

#endif