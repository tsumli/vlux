#include "pch.h"
//
#include "utils/io.h"
#include "utils/path.h"
#include "vlux/app.h"
#include "vlux/device_resource/device_resource.h"

int main() {
    const auto config_path = vlux::GetCurrentDir() / "config.json";
    const auto config = vlux::ReadJsonFile(config_path);

    // unpack config
    const auto spdlog_level = config.at("spdlog_level").get<std::string>();
    const auto width = config.at("width").get<uint32_t>();
    const auto height = config.at("height").get<uint32_t>();
    const auto vsync = config.at("vsync").get<bool>();
    const auto scene_name = config.at("scene").get<std::string>();

    // setup logger lovel
    if (spdlog_level == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (spdlog_level == "debug") {
        spdlog::set_level(spdlog::level::debug);
    }

    auto window = vlux::Window(width, height, "vlux");
    auto device_resource = vlux::DeviceResource(window, vsync);
    auto app = vlux::App(device_resource, scene_name);
    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}