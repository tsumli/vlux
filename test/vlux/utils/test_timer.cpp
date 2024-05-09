#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "vlux/utils/timer.h"

TEST_CASE("Timer", "[utils,time]") {
    auto timer = vlux::Timer();

    timer.Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto elapsed = timer.GetElapsedMilliseconds();
    REQUIRE_THAT(elapsed, Catch::Matchers::WithinAbs(100.f, 1.0f));
}