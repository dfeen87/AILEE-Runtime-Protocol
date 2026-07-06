#pragma once

#include "../core/wave_native_network_core.hpp"
#include <vector>
#include <cstdint>

namespace ailee {
namespace wnn {

enum class TransportVector {
    VECTOR_A,
    VECTOR_B
};

constexpr double K_IAT_NS_PER_RAD = 1000000.0;

void refract_wavefront(const WaveState& state_in, WaveState& state_out);

class PhyListener {
public:
    void listen(double delta_theta);
private:
    uint64_t logical_clock_ns_ = 0;
};

} // namespace wnn
} // namespace ailee
