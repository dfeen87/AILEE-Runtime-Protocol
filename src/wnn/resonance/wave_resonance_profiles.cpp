#include "wave_resonance_profiles.hpp"
#include <fstream>
#include <cstring>
#include <cmath>

namespace ailee {
namespace wnn {

void CalibrationMode::run_sweep() {
    // Generate deterministic profile based on purely deterministic math sweeps
    // Emulate finding optimal boundaries without random noise
    double optimal_omega = 1.0;
    double scan_step = 0.01;
    double max_stability = 0.0;

    // Deterministic loop to find local maximum stability based on standard Duffing rules
    for (double test_w = 0.5; test_w <= 1.5; test_w += scan_step) {
        // Mock stability function: inverted parabola centered at 1.05
        double stability = 1.0 - std::pow(test_w - 1.05, 2.0);
        if (stability > max_stability) {
            max_stability = stability;
            optimal_omega = test_w;
        }
    }

    HardwareProfile profile;
    profile.omega = optimal_omega;
    profile.alpha = -1.0;
    profile.beta = 1.0;
    profile.delta = 0.1;

    std::ofstream out("wnp_hw_profile.bin", std::ios::binary);
    if (out.is_open()) {
        out.write(reinterpret_cast<const char*>(&profile.omega), sizeof(double));
        out.write(reinterpret_cast<const char*>(&profile.alpha), sizeof(double));
        out.write(reinterpret_cast<const char*>(&profile.beta), sizeof(double));
        out.write(reinterpret_cast<const char*>(&profile.delta), sizeof(double));
        out.close();
    }
}

} // namespace wnn
} // namespace ailee
