#include "wave_phase_propagation.hpp"
#include <cmath>

namespace ailee {
namespace wnn {

void refract_wavefront(const WaveState& state_in, WaveState& state_out) {
    state_out = state_in;
    // Emulate internal resonance phase loss factor (e.g. 5% entropy degradation simulation limits)
    state_out.A *= 0.95;
}

void PhyListener::listen(double delta_theta) {
    double delay_ns = delta_theta * K_IAT_NS_PER_RAD;

    // Simulate spin-wait deterministic delay via logical monotonic counter advancement
    // rather than using std::chrono which is non-deterministic
    if (delay_ns > 0) {
        logical_clock_ns_ += static_cast<uint64_t>(delay_ns);
    }
}

} // namespace wnn
} // namespace ailee
