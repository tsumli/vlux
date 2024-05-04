#ifndef UTILS_PATH_H
#define UTILS_PATH_H

#include <filesystem>
#include <source_location>

namespace vlux {

inline std::filesystem::path GetCurrentDir(
    const std::source_location& location = std::source_location::current()) {
    return std::filesystem::path(location.file_name()).parent_path();
}

inline std::filesystem::path GetCurrentPath(
    const std::source_location& location = std::source_location::current()) {
    return std::filesystem::path(location.file_name());
}

}  // namespace vlux

#endif