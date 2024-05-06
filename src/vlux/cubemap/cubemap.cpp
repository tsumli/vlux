#include "cubemap.h"

namespace vlux {
bool ConvertEquirectanglerToCubeMap(std::array<std::vector<float>, 6>& cubemap_pixels,
                                    int& cubemap_width, int& cubemap_height,
                                    const std::vector<float>& equirectangular_pixels,
                                    const int equirectangular_width,
                                    const int equirectangular_height, const int channels) {
    // check equirectangler image size
    if (equirectangular_width != equirectangular_height * 2) {
        spdlog::debug("equirectanguler image width must be twice the height");
        return false;
    }

    // Set cubemap dimensions
    cubemap_width = equirectangular_height / 2;
    cubemap_height = cubemap_width;

    // Resize each face in the cubemap to store pixel data
    for (auto& face : cubemap_pixels) {
        face.resize(cubemap_width * cubemap_height * channels);
    }

    // Function to map equirectangular coordinates to cubemap coordinates
    auto mapCoords = [&](const double phi, const double theta, std::array<int, 3>& coord) {
        // Calculate x, y, z from phi and theta
        const auto x = std::cos(phi) * std::cos(theta);
        const auto y = std::cos(phi) * std::sin(theta);
        const auto z = std::sin(phi);

        // Determine which cubemap face the coordinate maps to
        const auto abs_x = std::abs(x);
        const auto abs_y = std::abs(y);
        const auto abs_z = std::abs(z);
        int face_idx;
        double uc, vc, max_axis;

        // Assign face index and corresponding u, v coordinates based on the major axis direction
        if (abs_x >= abs_y && abs_x >= abs_z) {
            max_axis = abs_x;
            uc = -y;
            vc = z;
            face_idx = x > 0 ? 0 : 1;  // +X or -X face
        } else if (abs_y >= abs_x && abs_y >= abs_z) {
            max_axis = abs_y;
            uc = x;
            vc = z;
            face_idx = y > 0 ? 2 : 3;  // +Y or -Y face
        } else {
            max_axis = abs_z;
            uc = x;
            vc = -y;
            face_idx = z > 0 ? 4 : 5;  // +Z or -Z face
        }

        // Normalize the coordinates
        uc /= max_axis;
        vc /= max_axis;

        // Convert range from -1 ... 1 to 0 ... cubemap dimension
        coord = {face_idx, static_cast<int>((uc + 1.0f) * 0.5f * cubemap_width - 0.5f),
                 static_cast<int>((vc + 1.0f) * 0.5f * cubemap_height - 0.5f)};
    };

    // Convert each pixel from the equirectangular image to the cubemap
    for (auto y = 0; y < equirectangular_height; ++y) {
        for (auto x = 0; x < equirectangular_width; ++x) {
            // Compute spherical coordinates phi and theta for current pixel
            const auto phi =
                std::numbers::pi_v<double> *
                (static_cast<double>(y) / equirectangular_height - 0.5f);  // -π/2 to π/2
            const auto theta = 2 * std::numbers::pi_v<double> * static_cast<double>(x) /
                               equirectangular_width;  // 0 to 2π

            // Determine which cubemap face this should map to
            std::array<int, 3> coord;
            mapCoords(phi, theta, coord);
            auto face_idx = coord[0];
            auto face_x = coord[1];
            auto face_y = coord[2];

            // Transfer pixel data
            for (auto channel = 0; channel < channels; ++channel) {
                cubemap_pixels[face_idx]
                              [face_y * cubemap_width * channels + face_x * channels + channel] =
                                  equirectangular_pixels[y * equirectangular_width * channels +
                                                         x * channels + channel];
            }
        }
    }

    // return success
    return true;
}

}  // namespace vlux