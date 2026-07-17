#ifndef AILEE_ENERGY_BATTERY_STATE_HPP
#define AILEE_ENERGY_BATTERY_STATE_HPP

#include <cmath>

namespace ailee {
namespace energy {

/**
 * @brief Represents the dual-state (Ψ physical, Φ informational) model of the battery.
 */
struct BatteryState {
    float soc = 1.0f;                      ///< State of Charge (0.0 to 1.0)
    float soh = 1.0f;                      ///< State of Health (0.0 to 1.0)
    float temperature_c = 25.0f;           ///< Battery temperature in °C
    float voltage_v = 400.0f;              ///< Battery voltage in Volts
    float current_a = 0.0f;                ///< Battery current in Amperes
    float estimated_cycles_used = 0.0f;    ///< Estimated equivalent full cycles used
    float estimated_cycles_remaining = 0.0f; ///< Estimated remaining full cycles before EOL
    float entropy_index = 0.0f;            ///< Informational entropy from HLV model (0.0 to 1.0)
    float degradation_rate = 0.0f;         ///< Rate of degradation (capacity loss per cycle)
    float regen_capacity = 1.0f;           ///< Normalized regenerative braking capacity factor (0.0 to 1.0)
    float discharge_capacity = 1.0f;       ///< Normalized discharge capacity factor (0.0 to 1.0)

    BatteryState() = default;

    /**
     * @brief Simple check of state validity.
     */
    bool is_valid() const {
        return std::isfinite(soc) && soc >= 0.0f && soc <= 1.0f &&
               std::isfinite(soh) && soh >= 0.0f && soh <= 1.0f &&
               std::isfinite(temperature_c) &&
               std::isfinite(voltage_v) && voltage_v > 0.0f &&
               std::isfinite(current_a) &&
               std::isfinite(estimated_cycles_used) && estimated_cycles_used >= 0.0f &&
               std::isfinite(estimated_cycles_remaining) &&
               std::isfinite(entropy_index) && entropy_index >= 0.0f &&
               std::isfinite(degradation_rate) &&
               std::isfinite(regen_capacity) && regen_capacity >= 0.0f &&
               std::isfinite(discharge_capacity) && discharge_capacity >= 0.0f;
    }
};

} // namespace energy
} // namespace ailee

#endif // AILEE_ENERGY_BATTERY_STATE_HPP
