#ifndef AILEE_ENERGY_HLV_PHYSICS_HPP
#define AILEE_ENERGY_HLV_PHYSICS_HPP

#include <array>
#include <cmath>
#include <cassert>
#include <algorithm>

namespace ailee {
namespace energy {

// Physical Constants from HLV theory
constexpr double PLANCK_REDUCED = 1.054571817e-34;  ///< ℏ (J·s)
constexpr double BOLTZMANN = 1.380649e-23;          ///< k_B (J/K)
constexpr double ELECTRON_CHARGE = 1.602176634e-19; ///< e (C)

/**
 * @brief Lightweight 4x4 matrix representing the effective metric g^eff_μν
 */
class Matrix4x4 {
public:
    std::array<std::array<double, 4>, 4> data;

    Matrix4x4() {
        // Initialize to Minkowski metric (-1, 1, 1, 1) by default
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                data[i][j] = (i == j) ? ((i == 0) ? -1.0 : 1.0) : 0.0;
            }
        }
    }

    double& operator()(int i, int j) {
        assert(i >= 0 && i < 4 && j >= 0 && j < 4 && "Matrix4x4 index out of bounds");
        return data[i][j];
    }

    const double& operator()(int i, int j) const {
        assert(i >= 0 && i < 4 && j >= 0 && j < 4 && "Matrix4x4 index out of bounds");
        return data[i][j];
    }

    double trace() const {
        return data[0][0] + data[1][1] + data[2][2] + data[3][3];
    }

    bool is_stable() const {
        double tr = trace();
        return std::isfinite(tr) && std::abs(tr) < 1e6;
    }
};

/**
 * @brief The internal HLV physical/informational state representation
 */
struct HLVState {
    // Physical state Ψ (directly measurable)
    double voltage = 0.0;
    double current = 0.0;
    double temperature = 25.0;
    double state_of_charge = 1.0;

    // Informational state Φ (computed/inferred)
    double entropy = 0.0;
    double cycle_count = 0.0;
    double degradation = 0.0;
    double phi_magnitude = 0.0;

    // Coupling parameters & metrics
    double lambda = 1e-6;
    Matrix4x4 g_eff;
    std::array<double, 4> grad_phi = {0.0, 0.0, 0.0, 0.0};

    // Energy tracking (Landauer compliance)
    double energy_psi = 0.0;
    double energy_phi = 0.0;
    double energy_metric = 0.0;
    double energy_total = 0.0;

    // Coulomb counting
    double charge_throughput_ah = 0.0;
    double capacity_fade = 0.0;

    // Timestamps
    double time = 0.0;
    double last_update = 0.0;

    HLVState() = default;

    bool is_valid() const {
        return std::isfinite(voltage) &&
               std::isfinite(current) &&
               std::isfinite(temperature) &&
               std::isfinite(state_of_charge) && state_of_charge >= 0.0 && state_of_charge <= 1.0 &&
               std::isfinite(degradation) && degradation >= 0.0 && degradation <= 1.0 &&
               g_eff.is_stable();
    }
};

// Safe Math Guards for HLV Physics / Energy Engine
inline double safe_div(double numerator, double denominator) {
    if (std::abs(denominator) < 1e-8) {
        return (denominator >= 0.0) ? (numerator / 1e-8) : (numerator / -1e-8);
    }
    return numerator / denominator;
}

inline float safe_div_f(float numerator, float denominator) {
    if (std::abs(denominator) < 1e-8f) {
        return (denominator >= 0.0f) ? (numerator / 1e-8f) : (numerator / -1e-8f);
    }
    return numerator / denominator;
}

inline double safe_exp(double val) {
    double clamped = std::clamp(val, -50.0, 50.0);
    return std::exp(clamped);
}

inline float safe_exp_f(float val) {
    float clamped = std::clamp(val, -50.0f, 50.0f);
    return std::exp(clamped);
}

} // namespace energy
} // namespace ailee

#endif // AILEE_ENERGY_HLV_PHYSICS_HPP
