#ifndef CUBEMAP_CUBEMAP_H
#define CUBEMAP_CUBEMAP_H
#include "pch.h"
//
#include "utils/io.h"

namespace vlux {

class CubeMapImage {
   public:
    CubeMapImage(const std::filesystem::path& path) {
        // check exr
        if (path.extension() != ".exr") {
            throw std::runtime_error("failed to load cubemap image!");
        }
        // check existence
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error(fmt::format("file: {} does not exist", path.string()));
        }
        ReadEXR(path, pixels_, width_, height_, channels_);
    }

   private:
    std::vector<float> pixels_;
    int width_;
    int height_;
    int channels_;
};

class CubeMap {
   public:
    CubeMap(const std::filesystem::path& path) : cube_map_image_(path) {}
    CubeMap() = delete;
    ~CubeMap() = default;
    CubeMap(const CubeMap&) = delete;
    CubeMap& operator=(const CubeMap&) = delete;
    CubeMap(CubeMap&&) = default;
    CubeMap& operator=(CubeMap&&) = default;

   private:
    CubeMapImage cube_map_image_;
};

}  // namespace vlux

#endif  // CUBEMAP_CUBEMAP_H