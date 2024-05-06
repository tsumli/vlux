#ifndef CUBEMAP_CUBEMAP_H
#define CUBEMAP_CUBEMAP_H
#include "pch.h"
//
#include "utils/io.h"

namespace vlux {

/**
 * @brief Convert equirectangler image to cubemap images
 *
 * @param cubemap_pixels 6 faces of cubemap images
 * @param cubemap_width Cubemap image width
 * @param cubemap_height Cubemap image height
 * @param equirectanler_pixels Equirectangler image pixels
 * @param equirentangler_width Equirectangler image width
 * @param equirectanler_height Equirectangler image height
 * @param channels Number of channels
 * @return true Success
 * @return false Failed
 */
bool ConvertEquirectanglerToCubeMap(std::array<std::vector<float>, 6>& cubemap_pixels,
                                    int& cubemap_width, int& cubemap_height,
                                    const std::vector<float>& equirectanguler_pixels,
                                    const int equirectangular_width,
                                    const int equirectangular_height, const int channels);

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
        ReadEXR(path, equirectangler_pixels_, equirentanglar_width_, equirectangler_height_,
                channels_);

        // convert equirectangler image to cubemap images
        ConvertEquirectanglerToCubeMap(cubemap_pixels_, cubemap_width_, cubemap_height_,
                                       equirectangler_pixels_, equirentanglar_width_,
                                       equirectangler_height_, channels_);

        // save pixels to exr
        for (size_t face = 0; face < cubemap_pixels_.size(); ++face) {
            std::filesystem::path face_path = path;
            face_path.replace_extension(fmt::format(".face{}.exr", face));
            WriteEXR(face_path, cubemap_pixels_[face], cubemap_width_, cubemap_height_, channels_);
        }
    }

   private:
    int channels_;

    // equirectangler image
    std::vector<float> equirectangler_pixels_;
    int equirentanglar_width_;
    int equirectangler_height_;

    // converted cubemap images (6 faces)
    std::array<std::vector<float>, 6> cubemap_pixels_;
    int cubemap_width_;
    int cubemap_height_;
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