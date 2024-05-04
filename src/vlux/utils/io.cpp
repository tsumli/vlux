#include "io.h"
//
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

namespace vlux {
void ReadEXR(const std::filesystem::path& filename, std::vector<float>& pixels, int& width,
             int& height, int& channels) {
    const auto exr_filename = filename.string();
    float* rgba = nullptr;
    const char* err = nullptr;
    const auto ret = LoadEXR(&rgba, &width, &height, exr_filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        throw std::runtime_error(fmt::format("failed to load EXR file: {}", err));
    }
    channels = 4;
    pixels.resize(width * height * channels);
    std::memcpy(pixels.data(), rgba, width * height * channels * sizeof(float));
    free(rgba);
}

}  // namespace vlux