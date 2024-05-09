#include <catch2/catch_test_macros.hpp>

#include "vlux/utils/path.h"

TEST_CASE("GetCurrentPath", "[utils]") {
    const auto path = vlux::GetCurrentPath();
    std::vector<std::filesystem::path> components;
    for (const auto& component : path) {
        components.push_back(component);
    }

    std::filesystem::path result;
    for (auto i = std::max(0uz, static_cast<std::size_t>(components.size()) - 4);
         i < components.size(); ++i) {
        result /= components[i];
    }

    REQUIRE(result == std::filesystem::path("test/vlux/utils/test_path.cpp"));
}

TEST_CASE("GetCurrentDir", "[utils]") {
    const auto path = vlux::GetCurrentDir();
    std::vector<std::filesystem::path> components;
    for (const auto& component : path) {
        components.push_back(component);
    }

    std::filesystem::path result;
    for (auto i = std::max(0uz, static_cast<std::size_t>(components.size()) - 3);
         i < components.size(); ++i) {
        result /= components[i];
    }

    REQUIRE(result == std::filesystem::path("test/vlux/utils"));
}