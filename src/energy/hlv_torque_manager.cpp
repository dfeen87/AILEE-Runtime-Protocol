#include "ailee/energy/hlv_physics.hpp"
#include "ailee/energy/energy_engine.hpp"
#include <cmath>
#include <algorithm>

namespace ailee {
namespace energy {

/**
 * @brief Helper function to compute the torque scaling factor under physical and informational stress.
 * Integrates degradation, informational entropy, and thermal thresholds.
 */
float compute_hlv_torque_scale(const HLVState& state, float max_temperature_c) {
    // 1. Degradation Scaling (Health limit)
    float health_scale = 1.0f - static_cast<float>(state.degradation);
    health_scale = std::clamp(health_scale, 0.1f, 1.0f);

    // 2. Entropy / Informational Scaling (Stress limit)
    float entropy_scale = 1.0f - 0.3f * static_cast<float>(state.entropy);
    entropy_scale = std::clamp(entropy_scale, 0.5f, 1.0f);

    // 3. Thermal Derating (Thermal limit)
    float thermal_scale = 1.0f;
    float start_derating_temp = max_temperature_c - 15.0f; // begin derating 15 degrees before max temp
    if (state.temperature > start_derating_temp) {
        float temp_excess = static_cast<float>(state.temperature) - start_derating_temp;
        thermal_scale = 1.0f - (temp_excess / 15.0f);
        thermal_scale = std::clamp(thermal_scale, 0.05f, 1.0f); // don't go to absolute zero, keep minimal limp home torque
    }

    // 4. Combined torque limit
    float combined_scale = health_scale * entropy_scale * thermal_scale;
    return std::clamp(combined_scale, 0.05f, 1.0f);
}

/**
 * @brief Helper function to compute regen efficiency capacity based on dual-state metrics.
 */
float compute_hlv_regen_capacity(const HLVState& state, float max_temperature_c) {
    // Regen capacity is highly restricted at high SoC (to prevent overcharge) and high temperature
    float soc_factor = 1.0f - static_cast<float>(state.state_of_charge);
    soc_factor = std::clamp(soc_factor, 0.0f, 1.0f);

    float temp_factor = 1.0f;
    float start_regen_derating_temp = max_temperature_c - 20.0f;
    if (state.temperature > start_regen_derating_temp) {
        float temp_excess = static_cast<float>(state.temperature) - start_regen_derating_temp;
        temp_factor = 1.0f - (temp_excess / 20.0f);
        temp_factor = std::clamp(temp_factor, 0.0f, 1.0f);
    }

    // Metric stability factor
    double metric_deviation = std::abs(state.g_eff.trace() - 2.0);
    float metric_factor = safe_div_f(1.0f, 1.0f + static_cast<float>(metric_deviation));

    float combined_regen = soc_factor * temp_factor * metric_factor;
    return std::clamp(combined_regen, 0.0f, 1.0f);
}

} // namespace energy
} // namespace ailee
