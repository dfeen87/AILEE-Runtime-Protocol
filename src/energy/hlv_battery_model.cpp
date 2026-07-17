#include "ailee/energy/hlv_physics.hpp"
#include "ailee/energy/energy_engine.hpp"
#include <cmath>
#include <algorithm>

namespace ailee {
namespace energy {

// Implement the core metric/geometric coupling of Krüger's dual-state model

void EnergyEngine::compute_phi_gradients() {
    // Map battery parameters to effective spacetime coordinates:
    // grad_phi[0]: Temporal gradient (how Φ changes with time)
    // grad_phi[1]: SoC gradient (how info changes with charge state)
    // grad_phi[2]: Temperature gradient (thermal coupling)
    // grad_phi[3]: Cycle gradient (history coupling)

    // temporal gradient is proportional to overall Φ magnitude and decay
    state_.grad_phi[0] = -config_.phi_decay_rate * state_.phi_magnitude;

    // SoC gradient
    state_.grad_phi[1] = state_.entropy * (1.0 - state_.state_of_charge);

    // Temperature gradient
    double temp_factor = safe_div(state_.temperature, config_.max_temperature_c);
    state_.grad_phi[2] = temp_factor * state_.degradation;

    // Cycle count gradient (history coupling)
    state_.grad_phi[3] = std::sqrt(std::max(0.0, state_.cycle_count)) * 0.01;
}

void EnergyEngine::compute_effective_metric() {
    // Start with Minkowski background (-1, 1, 1, 1)
    state_.g_eff = Matrix4x4();

    // Modulate with HLV coupling term: g_eff_μν = g_μν + λ * ∂_μΦ * ∂_νΦ
    for (int mu = 0; mu < 4; ++mu) {
        for (int nu = 0; nu < 4; ++nu) {
            state_.g_eff(mu, nu) += state_.lambda * state_.grad_phi[mu] * state_.grad_phi[nu];
        }
    }
}

} // namespace energy
} // namespace ailee
