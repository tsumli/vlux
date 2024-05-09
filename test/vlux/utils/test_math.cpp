#include <catch2/catch_test_macros.hpp>
//
#include "vlux/utils/math.h"

TEST_CASE("Math::Round", "[utils, math]") {
    REQUIRE(vlux::RoundDivUp(13, 13) == 1);
    REQUIRE(vlux::RoundDivUp(10, 14) == 1);
    REQUIRE(vlux::RoundDivUp(29, 15) == 2);
    REQUIRE(vlux::RoundUp(13, 13) == 13);
    REQUIRE(vlux::RoundUp(7, 15) == 15);
    REQUIRE(vlux::RoundUp(29, 15) == 30);
}