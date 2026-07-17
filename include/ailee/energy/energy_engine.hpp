#ifndef AILEE_ENERGY_ENGINE_HPP
#define AILEE_ENERGY_ENGINE_HPP

#include "ailee/energy/energy_config.hpp"
#include "ailee/energy/battery_state.hpp"
#include "ailee/energy/battery_advisory.hpp"
#include "ailee/energy/hlv_physics.hpp"

namespace ailee {
namespace energy {

/**
 * @brief Core computational engine implementing the dual-state HLV physics battery optimization.
 */
class EnergyEngine {
public:
    EnergyEngine();
    explicit EnergyEngine(const EnergyConfig& config);

    /**
     * @brief Initialize the engine with a configuration.
     */
    void init(const EnergyConfig& config);

    /**
     * @brief Update physical state and compute optimal HLV recommendations.
     * Performs dual-state updates, degradation tracking, charge optimization,
     * thermal stress modeling, and returns the RAW unblended advisory.
     */
    BatteryAdvisory update(float voltage, float current, float temperature, float soc, float dt);

    /**
     * @brief Computes the final blended advisory based on confidence and safety constraints.
     * Incorporates deterministic fallback scaling and linear blending in the grace band.
     */
    BatteryAdvisory compute_blended_advisory(const BatteryAdvisory& raw_advisory, float confidence) const;

    /**
     * @brief Gets the current estimated state of the battery.
     */
    BatteryState get_current_state() const;

    /**
     * @brief Gets the current engine configuration.
     */
    const EnergyConfig& get_config() const { return config_; }

private:
    // Helper mathematical routines from HLV theory
    void compute_phi_gradients();
    void compute_effective_metric();
    void update_phi(float dt);
    void compute_energies();

    // Predictions and Optimizations
    float predict_degradation_rate() const;
    float predict_cycles_remaining() const;
    float compute_risk_score() const;

    EnergyConfig config_;
    HLVState state_;
    bool initialized_ = false;

    // Initial and accumulated energy values for conservation tracking
    double initial_energy_ = 0.0;
    double cumulative_energy_error_ = 0.0;
};

/**
 * @brief Top-level orchestrator class for the AILEE Energy Runtime domain.
 * Wraps EnergyEngine and provides a clean integration point.
 */
class AILEEEnergyRuntime {
public:
    AILEEEnergyRuntime();
    explicit AILEEEnergyRuntime(const EnergyConfig& config);

    void init(const EnergyConfig& config);

    /**
     * @brief Run a single epoch/tick cycle of the battery optimization.
     * Updates internal state and returns a blended, safety-clamped Advisory.
     */
    BatteryAdvisory run_cycle(float voltage, float current, float temperature, float soc, float dt);

    BatteryState get_state() const;
    const EnergyConfig& get_config() const;

private:
    EnergyEngine engine_;
};

} // namespace energy
} // namespace ailee

#endif // AILEE_ENERGY_ENGINE_HPP
