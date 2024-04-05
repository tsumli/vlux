#include "vlux/app.h"
#include "vlux/device_resource/device_resource.h"

int main() {
    auto window = vlux::Window(1920, 1080, "vlux");
    auto device_resource = vlux::DeviceResource(window);
    auto app = vlux::App(device_resource);
    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}