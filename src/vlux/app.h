#ifndef APP_H
#define APP_H
#include <memory>

#include "camera.h"
#include "control.h"
#include "device_resource/device_resource.h"
#include "draw/draw_strategy.h"
#include "frame_timer.h"
#include "gui.h"
#include "scene/scene.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux {

class App {
   public:
    App() = delete;
    App(DeviceResource& device_resource);
    ~App();
    void Run() { MainLoop(); }

   private:
    void MainLoop();
    void CreateScene();
    void DrawFrame();

    DeviceResource& device_resource_;
    std::optional<Gui> gui_ = std::nullopt;
    std::optional<Control> control_ = std::nullopt;
    std::optional<Scene> scene_ = std::nullopt;
    std::optional<Camera> camera_ = std::nullopt;

    // UBO
    UniformBuffer<TransformParams> transform_ubo_;

    // Draw
    std::unique_ptr<draw::DrawStrategy> draw_ = nullptr;

    int current_frame_ = 0;

    // frame timer
    FrameTimer frame_timer_;
};
}  // namespace vlux

#endif