#include "ailee/energy/hlv_physics.hpp"
#include "ailee/energy/energy_engine.hpp"
#include <cmath>
#include <algorithm>

namespace ailee {
namespace energy {

void EnergyEngine::update_phi(float dt) {
    // Entropy increases with cycling and temperature
    double temp_contrib = std::max(0.0, state_.temperature - 25.0) / 35.0;
    double current_contrib = std::abs(state_.current) / 100.0; // Normalized current contribution

    state_.entropy += dt * config_.entropy_weight * (temp_contrib + current_contrib);
    state_.entropy = std::min(state_.entropy, 1.0);

    // Coulomb counting for cycle tracking
    double charge_delta = safe_div(std::abs(state_.current) * dt, 3600.0); // Ah
    state_.charge_throughput_ah += charge_delta;
    double prev_cycle_count = state_.cycle_count;
    state_.cycle_count = safe_div(state_.charge_throughput_ah, 2.0 * config_.nominal_capacity_ah);

    // Degradation update
    double cycle_delta = state_.cycle_count - prev_cycle_count;
    double base_degradation = cycle_delta * 0.0001; // Baseline degradation rate
    double thermal_degradation = temp_contrib * 0.0005 * dt;
    double hlv_correction = state_.g_eff.trace() * 0.00001; // Metric distortion penalty

    state_.degradation += base_degradation + thermal_degradation + hlv_correction;
    state_.degradation = std::clamp(state_.degradation, 0.0, 1.0);

    state_.capacity_fade = state_.degradation;

    // Magnitude of information state vector Φ
    state_.phi_magnitude = std::sqrt(state_.entropy * state_.entropy + state_.degradation * state_.degradation);
}

void EnergyEngine::compute_energies() {
    // Physical stored energy
    state_.energy_psi = state_.voltage * state_.state_of_charge * config_.nominal_capacity_ah * 3600.0;

    // Informational energy (Landauer's Principle: k_B * T * ln(2) per bit)
    double temp_kelvin = state_.temperature + 273.15;
    double info_bits = state_.phi_magnitude * 1000.0; // scaled information bits representation
    state_.energy_phi = info_bits * BOLTZMANN * temp_kelvin * std::log(2.0);

    // Metric modulation energy
    double metric_deviation = std::abs(state_.g_eff.trace() - 2.0);
    state_.energy_metric = metric_deviation * state_.energy_psi * 1e-6;

    // Total energy sum
    state_.energy_total = state_.energy_psi + state_.energy_phi + state_.energy_metric;
}

} // namespace energy
} // namespace ailee
