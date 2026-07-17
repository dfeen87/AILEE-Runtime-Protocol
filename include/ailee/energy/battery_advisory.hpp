#ifndef AILEE_ENERGY_BATTERY_ADVISORY_HPP
#define AILEE_ENERGY_BATTERY_ADVISORY_HPP

#include <cmath>

namespace ailee {
namespace energy {

/**
 * @brief Represents optimal battery management recommendations and safe operational envelopes.
 */
struct BatteryAdvisory {
    float torque_scale = 1.0f;                    ///< Multiplier for torque/regen scaling (0.0 to 1.0)
    float recommended_charge_current_a = 0.0f;    ///< Recommended maximum charging current in Amperes
    float recommended_voltage_v = 400.0f;         ///< Recommended optimal battery terminal voltage in Volts
    float max_safe_temperature_c = 60.0f;         ///< Maximum safe temperature boundary in °C
    float recommended_charge_time_minutes = 0.0f; ///< Estimated time to reach full charge at recommended current
    float recommended_discharge_rate_c = 1.0f;    ///< Recommended continuous discharge C-rate (e.g. 1.0C, 2.0C)
    float risk_score = 0.0f;                       ///< Estimated risk score (0.0 = safe, 1.0 = pre-fault condition)
    float confidence = 1.0f;                       ///< Trust metric of the prediction (0.0 to 1.0)
    bool allow_fast_charge = true;                ///< True if fast-charging mode is recommended/safe
    bool require_derating = false;                 ///< True if motor torque/power must be derated due to thermal/degradation stress

    BatteryAdvisory() = default;

    /**
     * @brief Blends this advisory with another based on confidence-weighted rules.
     */
    static BatteryAdvisory blend(const BatteryAdvisory& adv, const BatteryAdvisory& fallback, float weight, float fallback_scale) {
        BatteryAdvisory result;
        // Case 1, 2, or 3 based on weight 'w' passed down from the engine
        result.torque_scale = weight * adv.torque_scale + (1.0f - weight) * fallback.torque_scale * fallback_scale;
        result.recommended_charge_current_a = weight * adv.recommended_charge_current_a + (1.0f - weight) * fallback.recommended_charge_current_a * fallback_scale;
        result.recommended_voltage_v = weight * adv.recommended_voltage_v + (1.0f - weight) * fallback.recommended_voltage_v * fallback_scale;

        // Temperatures and times are envelopes/scores that we can blend linearly
        result.max_safe_temperature_c = weight * adv.max_safe_temperature_c + (1.0f - weight) * fallback.max_safe_temperature_c;
        result.recommended_charge_time_minutes = weight * adv.recommended_charge_time_minutes + (1.0f - weight) * fallback.recommended_charge_time_minutes;
        result.recommended_discharge_rate_c = weight * adv.recommended_discharge_rate_c + (1.0f - weight) * fallback.recommended_discharge_rate_c * fallback_scale;

        result.risk_score = weight * adv.risk_score + (1.0f - weight) * fallback.risk_score;
        result.confidence = weight * adv.confidence + (1.0f - weight) * fallback.confidence;

        // Boolean flags
        result.allow_fast_charge = (weight >= 0.5f) ? adv.allow_fast_charge : fallback.allow_fast_charge;
        result.require_derating = (weight >= 0.5f) ? adv.require_derating : fallback.require_derating;
        if (weight < 0.5f && fallback_scale < 1.0f) {
            result.require_derating = true; // Force derating under fallback conditions
        }

        return result;
    }
};

} // namespace energy
} // namespace ailee

#endif // AILEE_ENERGY_BATTERY_ADVISORY_HPP
