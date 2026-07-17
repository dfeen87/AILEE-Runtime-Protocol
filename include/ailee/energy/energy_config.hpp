#ifndef AILEE_ENERGY_CONFIG_HPP
#define AILEE_ENERGY_CONFIG_HPP

#include <string>
#include <cmath>
#include "ailee/energy/battery_advisory.hpp"

namespace ailee {
namespace energy {

/**
 * @brief Configuration settings for the AILEE Energy Runtime module.
 */
struct EnergyConfig {
    // Required thresholds and config settings
    float min_confidence_threshold = 0.20f;
    float grace_confidence_threshold = 0.10f;
    float fallback_position_scale = 0.1f;
    std::string config_version = "5.0.0-energy";

    // Standard battery parameters
    float nominal_capacity_ah = 75.0f;
    float nominal_voltage_v = 400.0f;
    float max_current_a = 500.0f;
    float max_temperature_c = 60.0f;
    float min_temperature_c = -20.0f;
    float lambda = 1e-6f;
    float entropy_weight = 0.5f;
    float phi_decay_rate = 0.001f;
    float tau_min = 0.01f;

    // Fallback baseline defaults (safe, conservative)
    float fallback_torque_scale = 0.3f;
    float fallback_charge_current_a = 10.0f;
    float fallback_voltage_v = 360.0f;
    float fallback_max_safe_temperature_c = 45.0f;
    float fallback_discharge_rate_c = 0.5f;

    EnergyConfig() = default;

    /**
     * @brief Validate configuration parameters.
     */
    bool validate() const {
        if (min_confidence_threshold < 0.0f || min_confidence_threshold > 1.0f) return false;
        if (grace_confidence_threshold < 0.0f || grace_confidence_threshold > 1.0f) return false;
        if (grace_confidence_threshold > min_confidence_threshold) return false;
        if (fallback_position_scale <= 0.0f || fallback_position_scale > 1.0f) return false;
        if (config_version.empty()) return false;

        if (nominal_capacity_ah <= 0.0f) return false;
        if (nominal_voltage_v <= 0.0f) return false;
        if (max_current_a <= 0.0f) return false;
        if (max_temperature_c <= min_temperature_c) return false;
        if (lambda <= 0.0f || lambda > 1e-3f) return false;
        if (entropy_weight < 0.0f || entropy_weight > 1.0f) return false;
        if (phi_decay_rate < 0.0f || phi_decay_rate > 1.0f) return false;
        if (tau_min <= 0.0f || tau_min > 1.0f) return false;

        if (fallback_torque_scale < 0.0f || fallback_torque_scale > 1.0f) return false;
        if (fallback_charge_current_a < 0.0f) return false;
        if (fallback_voltage_v <= 0.0f) return false;
        if (fallback_max_safe_temperature_c <= 0.0f) return false;
        if (fallback_discharge_rate_c < 0.0f) return false;

        return true;
    }

    /**
     * @brief Generate a default fallback advisory object based on configuration.
     */
    BatteryAdvisory get_fallback_advisory() const {
        BatteryAdvisory adv;
        adv.torque_scale = fallback_torque_scale;
        adv.recommended_charge_current_a = fallback_charge_current_a;
        adv.recommended_voltage_v = fallback_voltage_v;
        adv.max_safe_temperature_c = fallback_max_safe_temperature_c;
        adv.recommended_charge_time_minutes = 120.0f; // slower default charge
        adv.recommended_discharge_rate_c = fallback_discharge_rate_c;
        adv.risk_score = 0.5f;
        adv.confidence = 1.0f;
        adv.allow_fast_charge = false;
        adv.require_derating = true;
        return adv;
    }
};

} // namespace energy
} // namespace ailee

#endif // AILEE_ENERGY_CONFIG_HPP
