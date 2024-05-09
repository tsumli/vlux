#include "camera.h"

namespace vlux {
namespace {
glm::vec3 Vec4ToVec3(const glm::vec4& vec) { return {vec.x, vec.y, vec.z}; }
}  // namespace

Camera::Camera(const glm::vec3& pos, const glm::vec3& rot, const float width, const float height)
    : pos_(pos), rot_(rot) {
    constexpr auto kFov = glm::radians(60.0f);
    constexpr auto kNearZ = 0.1f;
    constexpr auto kFarZ = 1000.0f;
    world_matrix_ = glm::mat4(1.0f);  // identity matrix
    proj_matrix_ = glm::perspectiveFovRH_ZO(kFov, width, height, kNearZ, kFarZ);
    proj_matrix_[1][1] *= -1;
}

void Camera::UpdatePosition(const int move_forward, const int move_right, const int move_up,
                            const float gain) {
    pos_ += Vec4ToVec3(lookat_) * gain * static_cast<float>(move_forward);
    pos_ -= Vec4ToVec3(cam_right_) * gain * static_cast<float>(move_right);
    pos_ += Vec4ToVec3(cam_up_) * gain * static_cast<float>(move_up);
}

void Camera::UpdateRotation(const float cur_right, const float cur_up, const float gain) {
    rot_.x -= gain * cur_right;
    rot_.y += gain * cur_up;

    auto clamp = [](const float v) {
        const auto pi = std::numbers::pi_v<float>;
        constexpr auto kEpsion = 0.01f;
        return std::clamp(v, -pi / 2.0f + kEpsion, pi / 2.0f - kEpsion);
    };
    rot_.y = clamp(rot_.y);

    const auto pitch_matrix = glm::rotate(glm::mat4(1.0f), rot_.y, glm::vec3(1, 0, 0));
    const auto yaw_matrix = glm::rotate(glm::mat4(1.0f), rot_.x, glm::vec3(0, 1, 0));
    const auto rotation_matrix = yaw_matrix * pitch_matrix;

    lookat_ = glm::normalize(rotation_matrix * kLookat);
    cam_right_ = rotation_matrix * kCamRight;
    cam_up_ = glm::vec4(glm::cross(Vec4ToVec3(lookat_), Vec4ToVec3(cam_right_)), 0.0f);
}

glm::mat4x4 Camera::CreateViewMatrix() const {
    const auto lookat = Vec4ToVec3(lookat_);
    const auto cam_up = Vec4ToVec3(cam_up_);
    auto view_matrix = glm::lookAt(pos_, pos_ + lookat, cam_up);
    return view_matrix;
}

TransformParams Camera::CreateTransformParams() {
    auto view_matrix = CreateViewMatrix();
    const auto proj_to_world_matrix = glm::inverse(proj_matrix_ * view_matrix);

    return TransformParams{
        .world = world_matrix_,
        .view_proj = proj_matrix_ * view_matrix,
        .world_view_proj = proj_matrix_ * view_matrix * world_matrix_,
        .proj_to_world = proj_to_world_matrix,
    };
}

}  // namespace vlux