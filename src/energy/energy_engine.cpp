#include "ailee/energy/energy_engine.hpp"
#include "core/Logging.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

namespace ailee {
namespace energy {

// Declare helpers implemented in hlv_torque_manager.cpp
float compute_hlv_torque_scale(const HLVState& state, float max_temperature_c);
float compute_hlv_regen_capacity(const HLVState& state, float max_temperature_c);

// ============================================================================
// EnergyEngine Implementation
// ============================================================================

EnergyEngine::EnergyEngine() {
    init(EnergyConfig());
}

EnergyEngine::EnergyEngine(const EnergyConfig& config) {
    init(config);
}

void EnergyEngine::init(const EnergyConfig& config) {
    if (!config.validate()) {
        throw std::invalid_argument("Invalid EnergyConfig parameters provided to EnergyEngine");
    }
    config_ = config;
    state_ = HLVState();
    state_.lambda = config_.lambda;
    initial_energy_ = 0.0;
    cumulative_energy_error_ = 0.0;
    initialized_ = true;
}

BatteryAdvisory EnergyEngine::update(float voltage, float current, float temperature, float soc, float dt) {
    if (!initialized_) {
        throw std::runtime_error("EnergyEngine not initialized");
    }

    auto logger = ailee::log::getLogger("EnergyEngine");

    // Developer Assertions
    assert(std::isfinite(voltage) && "voltage must be finite");
    assert(std::isfinite(current) && "current must be finite");
    assert(std::isfinite(temperature) && "temperature must be finite");
    assert(std::isfinite(soc) && "soc must be finite");
    assert(std::isfinite(dt) && "dt must be finite");

    // Runtimeguards / checks for invalid or non-finite inputs
    if (!std::isfinite(voltage) || !std::isfinite(current) || !std::isfinite(temperature) ||
        !std::isfinite(soc) || !std::isfinite(dt) || dt <= 0.0f) {
        LOG_WARN(logger, "Non-finite or invalid input detected in EnergyEngine::update. Triggering baseline fallback.");
        BatteryAdvisory fallback_adv = config_.get_fallback_advisory();
        fallback_adv.confidence = 0.0f; // Low/invalid confidence
        fallback_adv.risk_score = 1.0f; // Pre-fault indicator
        return fallback_adv;
    }

    // Skip update if interval is below tau_min (bit-erasure threshold)
    if (dt < config_.tau_min) {
        // Return current advisory equivalent with zeroed increments
        BatteryAdvisory adv;
        adv.confidence = 0.0f; // invalid/stale
        return adv;
    }

    // Boundary validations and clamps
    bool input_clamped = false;
    if (soc < 0.0f || soc > 1.0f) {
        LOG_WARN(logger, "State of Charge out of [0, 1] range. Clamping.");
        soc = std::clamp(soc, 0.0f, 1.0f);
        input_clamped = true;
    }

    if (std::abs(current) > config_.max_current_a) {
        LOG_WARN(logger, "Current exceeds max physical boundary. Clamping.");
        current = std::copysign(config_.max_current_a, current);
        input_clamped = true;
    }
    if (temperature > config_.max_temperature_c) {
        LOG_WARN(logger, "Temperature exceeds max safe boundary. Clamping.");
        temperature = config_.max_temperature_c;
        input_clamped = true;
    } else if (temperature < config_.min_temperature_c) {
        LOG_WARN(logger, "Temperature below min safe boundary. Clamping.");
        temperature = config_.min_temperature_c;
        input_clamped = true;
    }

    // Update physical state parameters
    state_.voltage = voltage;
    state_.current = current;
    state_.temperature = temperature;
    state_.state_of_charge = std::clamp(static_cast<double>(soc), 0.0, 1.0);

    // Advanced time track
    state_.time += dt;
    state_.last_update = state_.time;

    // Track state energy before updates
    double energy_before = state_.energy_total;

    // 1. Informational Φ State Update
    update_phi(dt);

    // 2. Gradients and Spacetime Metric Coupling
    compute_phi_gradients();
    compute_effective_metric();

    // 3. Energy Compliance (Landauer's Principle)
    compute_energies();

    // Cumulative conservation check
    double energy_error = std::abs(state_.energy_total - energy_before);
    cumulative_energy_error_ += energy_error;

    // --- Build RAW Advisory based on HLV math ---
    BatteryAdvisory adv;

    // Metric stability factor
    double metric_deviation = std::abs(state_.g_eff.trace() - 2.0);
    float metric_factor = safe_div_f(1.0f, 1.0f + static_cast<float>(metric_deviation));

    // Torque scaling and regen capacity loops
    adv.torque_scale = compute_hlv_torque_scale(state_, config_.max_temperature_c);

    // Recommended Charge Current (minimizing metric distortion)
    adv.recommended_charge_current_a = 100.0f * metric_factor * (1.0f - static_cast<float>(state_.degradation));
    adv.recommended_charge_current_a = std::clamp(adv.recommended_charge_current_a, 0.0f, config_.max_current_a);

    // Recommended Voltage Limit
    adv.recommended_voltage_v = config_.nominal_voltage_v * (1.0f + 0.1f * metric_factor);

    // Thermal stress modeling maximum safe temperature
    adv.max_safe_temperature_c = config_.max_temperature_c;

    // Estimated Charge Time
    float remaining_capacity_ah = (1.0f - static_cast<float>(state_.state_of_charge)) * config_.nominal_capacity_ah;
    adv.recommended_charge_time_minutes = safe_div_f(60.0f * remaining_capacity_ah, std::max(adv.recommended_charge_current_a, 1.0f));

    // Recommended continuous discharge C-rate
    adv.recommended_discharge_rate_c = 2.0f * metric_factor * (1.0f - static_cast<float>(state_.degradation));
    adv.recommended_discharge_rate_c = std::clamp(adv.recommended_discharge_rate_c, 0.1f, 5.0f);

    // Overall Risk Score based on stress indicators
    adv.risk_score = compute_risk_score();

    // Predict confidence based on metric stability & health degradation
    adv.confidence = metric_factor * (1.0f - static_cast<float>(state_.degradation));
    adv.confidence = std::clamp(adv.confidence, 0.0f, 1.0f);

    // Fast-charging and derating switches
    adv.allow_fast_charge = (state_.degradation < 0.4) && (state_.temperature < (config_.max_temperature_c - 10.0));
    adv.require_derating = (adv.torque_scale < 0.95f) || (state_.degradation > 0.3) || input_clamped;

    return adv;
}

BatteryAdvisory EnergyEngine::compute_blended_advisory(const BatteryAdvisory& raw_advisory, float confidence) const {
    float c_min = config_.min_confidence_threshold;
    float c_grace = config_.grace_confidence_threshold;
    float s_fallback = config_.fallback_position_scale;

    BatteryAdvisory fallback_baseline = config_.get_fallback_advisory();

    // Case 1 — high confidence (c >= c_min)
    if (confidence >= c_min) {
        return BatteryAdvisory::blend(raw_advisory, fallback_baseline, 1.0f, 1.0f);
    }

    // Case 2 — grace band (c_grace <= c < c_min)
    if (confidence >= c_grace) {
        float alpha = safe_div_f(confidence - c_grace, c_min - c_grace);
        return BatteryAdvisory::blend(raw_advisory, fallback_baseline, alpha, s_fallback);
    }

    // Case 3 — low confidence (c < c_grace)
    return BatteryAdvisory::blend(raw_advisory, fallback_baseline, 0.0f, s_fallback);
}

BatteryState EnergyEngine::get_current_state() const {
    BatteryState s;
    s.soc = static_cast<float>(state_.state_of_charge);
    s.soh = 1.0f - static_cast<float>(state_.degradation);
    s.soh = std::clamp(s.soh, 0.0f, 1.0f);
    s.temperature_c = static_cast<float>(state_.temperature);
    s.voltage_v = static_cast<float>(state_.voltage);
    s.current_a = static_cast<float>(state_.current);
    s.estimated_cycles_used = static_cast<float>(state_.cycle_count);
    s.estimated_cycles_remaining = predict_cycles_remaining();
    s.entropy_index = static_cast<float>(state_.entropy);
    s.degradation_rate = predict_degradation_rate();
    s.regen_capacity = compute_hlv_regen_capacity(state_, config_.max_temperature_c);
    s.discharge_capacity = compute_hlv_torque_scale(state_, config_.max_temperature_c);
    return s;
}

float EnergyEngine::predict_degradation_rate() const {
    // Proportional to metric trace
    double rate = state_.g_eff.trace() * 0.0001;
    return static_cast<float>(rate);
}

float EnergyEngine::predict_cycles_remaining() const {
    double degradation_rate = state_.g_eff.trace() * 0.0001;
    if (degradation_rate <= 1e-8) {
        return 5000.0f; // clean baseline limit
    }
    // Cycles until 80% SOH (0.2 degradation)
    double remaining = (0.2 - state_.degradation) / degradation_rate;
    return std::max(0.0f, static_cast<float>(remaining));
}

float EnergyEngine::compute_risk_score() const {
    float temp_factor = 0.0f;
    if (config_.max_temperature_c != 0.0f) {
        temp_factor = static_cast<float>(state_.temperature) / config_.max_temperature_c;
    }
    float deg_factor = static_cast<float>(state_.degradation);

    double metric_deviation = std::abs(state_.g_eff.trace() - 2.0);
    float metric_factor = std::min(1.0f, static_cast<float>(metric_deviation) / 10.0f);

    float combined = 0.3f * temp_factor + 0.4f * deg_factor + 0.3f * metric_factor;
    return std::clamp(combined, 0.0f, 1.0f);
}

// ============================================================================
// AILEEEnergyRuntime Implementation
// ============================================================================

AILEEEnergyRuntime::AILEEEnergyRuntime() : engine_() {}

AILEEEnergyRuntime::AILEEEnergyRuntime(const EnergyConfig& config) : engine_(config) {}

void AILEEEnergyRuntime::init(const EnergyConfig& config) {
    engine_.init(config);
}

BatteryAdvisory AILEEEnergyRuntime::run_cycle(float voltage, float current, float temperature, float soc, float dt) {
    BatteryAdvisory raw = engine_.update(voltage, current, temperature, soc, dt);
    return engine_.compute_blended_advisory(raw, raw.confidence);
}

BatteryState AILEEEnergyRuntime::get_state() const {
    return engine_.get_current_state();
}

const EnergyConfig& AILEEEnergyRuntime::get_config() const {
    return engine_.get_config();
}

} // namespace energy
} // namespace ailee
