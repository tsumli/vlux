#ifndef APP_H
#define APP_H

#include "camera.h"
#include "common/command_buffer.h"
#include "common/command_pool.h"
#include "control.h"
#include "device_resource/device_resource.h"
#include "draw/draw_strategy.h"
#include "frame_timer.h"
#include "gui.h"
#include "light.h"
#include "scene/scene.h"
#include "transform.h"
#include "uniform_buffer.h"
#include "utils/timer.h"

namespace vlux {

class App {
   public:
    App() = delete;
    App(DeviceResource& device_resource, const std::string_view scene_name);
    ~App();
    void Run() { MainLoop(); }

   private:
    void MainLoop();
    void CreateScene();
    void DrawFrame();

    DeviceResource& device_resource_;
    std::optional<Gui> gui_ = std::nullopt;
    std::optional<Scene> scene_ = std::nullopt;
    std::optional<Camera> camera_ = std::nullopt;
    std::optional<Control> control_ = std::nullopt;
    struct MouseConfig {
        float sens{0.001f};
    } mouse_config_;
    enum class MouseRightButtonState : uint8_t {
        kReleased,
        kTriggered,
        kCount,
    } mouse_right_button_state_ = MouseRightButtonState::kReleased;

    // UBO
    UniformBuffer<TransformParams> transform_ubo_;
    UniformBuffer<CameraParams> camera_ubo_;
    UniformBuffer<CameraMatrixParams> camera_matrix_ubo_;
    UniformBuffer<LightParams> light_ubo_;

    // objects
    std::vector<LightParams> lights_;

    // Draw
    std::string draw_mode_;
    std::unique_ptr<draw::DrawStrategy> draw_ = nullptr;

    // Resource
    std::optional<CommandPool> command_pool_ = std::nullopt;
    std::optional<CommandBuffer> command_buffer_ = std::nullopt;

    // frame timer
    FrameTimer frame_timer_;
    Timer timer_;

    // config
    const nlohmann::json config_;
    std::string scene_name_;
};
}  // namespace vlux

#endif