#include "math.h"

namespace {
bool IsPow2(uint32_t value) { return ((value - 1) & value) == 0; }
}  // namespace

namespace vlux {
uint32_t RoundUpPow2(uint32_t x, uint32_t s) {
    assert(IsPow2(s));
    return (x + s - 1) & (~(s - 1));
}
uint32_t RoundUp(uint32_t x, uint32_t s) { return RoundDivUp(x, s) * s; }
uint32_t RoundDivUp(uint32_t x, uint32_t s) { return ((x + s - 1) / s); }
}  // namespace vlux