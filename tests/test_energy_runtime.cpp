#include "ailee/energy/energy_engine.hpp"
#include "ailee/energy/battery_state.hpp"
#include "ailee/energy/battery_advisory.hpp"
#include <gtest/gtest.h>
#include <cmath>

#ifndef EXPECT_FLOAT_EQ
#define EXPECT_FLOAT_EQ(A, B) EXPECT_NEAR(A, B, 1e-5f)
#endif

using namespace ailee::energy;

TEST(EnergyRuntimeTest, StructDefaults) {
    BatteryState state;
    EXPECT_FLOAT_EQ(state.soc, 1.0f);
    EXPECT_FLOAT_EQ(state.soh, 1.0f);
    EXPECT_FLOAT_EQ(state.temperature_c, 25.0f);
    EXPECT_TRUE(state.is_valid());

    BatteryAdvisory advisory;
    EXPECT_FLOAT_EQ(advisory.torque_scale, 1.0f);
    EXPECT_TRUE(advisory.allow_fast_charge);
    EXPECT_FALSE(advisory.require_derating);
}

TEST(EnergyRuntimeTest, FallbackScalingAndBlendingCases) {
    EnergyConfig config;
    EnergyEngine engine(config);

    BatteryAdvisory raw;
    raw.torque_scale = 0.9f;
    raw.recommended_charge_current_a = 80.0f;
    raw.recommended_voltage_v = 420.0f;
    raw.max_safe_temperature_c = 60.0f;
    raw.recommended_charge_time_minutes = 30.0f;
    raw.recommended_discharge_rate_c = 3.0f;
    raw.risk_score = 0.1f;
    raw.confidence = 0.8f;
    raw.allow_fast_charge = true;
    raw.require_derating = false;

    BatteryAdvisory fallback = config.get_fallback_advisory();
    // Default fallback values:
    // fallback_torque_scale = 0.3f
    // fallback_charge_current_a = 10.0f
    // fallback_voltage_v = 360.0f
    // fallback_discharge_rate_c = 0.5f
    // fallback_position_scale = 0.1f

    // Case 1: High confidence (e.g. 0.5f >= min_confidence_threshold of 0.20f)
    {
        BatteryAdvisory blended = engine.compute_blended_advisory(raw, 0.5f);
        EXPECT_FLOAT_EQ(blended.torque_scale, raw.torque_scale);
        EXPECT_FLOAT_EQ(blended.recommended_charge_current_a, raw.recommended_charge_current_a);
        EXPECT_FLOAT_EQ(blended.recommended_voltage_v, raw.recommended_voltage_v);
        EXPECT_FLOAT_EQ(blended.max_safe_temperature_c, raw.max_safe_temperature_c);
        EXPECT_TRUE(blended.allow_fast_charge);
    }

    // Case 2: Grace band (e.g. 0.15f, which is exactly halfway between 0.10f and 0.20f)
    {
        // alpha = (0.15 - 0.10) / (0.20 - 0.10) = 0.5f
        // final = 0.5 * raw + 0.5 * fallback * 0.1
        BatteryAdvisory blended = engine.compute_blended_advisory(raw, 0.15f);

        float expected_torque = 0.5f * raw.torque_scale + 0.5f * fallback.torque_scale * config.fallback_position_scale;
        EXPECT_FLOAT_EQ(blended.torque_scale, expected_torque);

        float expected_current = 0.5f * raw.recommended_charge_current_a + 0.5f * fallback.recommended_charge_current_a * config.fallback_position_scale;
        EXPECT_FLOAT_EQ(blended.recommended_charge_current_a, expected_current);

        float expected_voltage = 0.5f * raw.recommended_voltage_v + 0.5f * fallback.recommended_voltage_v * config.fallback_position_scale;
        EXPECT_FLOAT_EQ(blended.recommended_voltage_v, expected_voltage);
    }

    // Case 3: Low confidence (e.g. 0.05f < grace_confidence_threshold of 0.10f)
    {
        // final = fallback * 0.1
        BatteryAdvisory blended = engine.compute_blended_advisory(raw, 0.05f);

        EXPECT_FLOAT_EQ(blended.torque_scale, fallback.torque_scale * config.fallback_position_scale);
        EXPECT_FLOAT_EQ(blended.recommended_charge_current_a, fallback.recommended_charge_current_a * config.fallback_position_scale);
        EXPECT_FLOAT_EQ(blended.recommended_voltage_v, fallback.recommended_voltage_v * config.fallback_position_scale);
        EXPECT_TRUE(blended.require_derating);
        EXPECT_FALSE(blended.allow_fast_charge);
    }
}

TEST(EnergyRuntimeTest, PhysicsModelOutputEvolution) {
    EnergyConfig config;
    // Set parameters for rapid evolution to test easily
    config.entropy_weight = 1.0f;
    config.lambda = 1e-4f;

    EnergyEngine engine(config);

    // Initial state check
    BatteryState initial = engine.get_current_state();
    EXPECT_FLOAT_EQ(initial.soc, 1.0f);
    EXPECT_FLOAT_EQ(initial.estimated_cycles_used, 0.0f);
    EXPECT_FLOAT_EQ(initial.entropy_index, 0.0f);

    // Simulate high load discharging
    // Update: voltage = 380V, current = 20.0A, temperature = 30°C, soc = 0.9f, dt = 0.1s (small values to avoid immediate saturation)
    BatteryAdvisory advisory1 = engine.update(380.0f, 20.0f, 30.0f, 0.9f, 0.1f);
    (void)advisory1;
    BatteryState state1 = engine.get_current_state();

    // Verify entropy accumulated and capacity/cycles started tracking
    EXPECT_GT(state1.entropy_index, 0.0f);
    EXPECT_LT(state1.entropy_index, 1.0f);
    EXPECT_GT(state1.estimated_cycles_used, 0.0f);
    EXPECT_LT(state1.soh, 1.0f); // degradation occurred

    // Run high temperature stress update
    BatteryAdvisory advisory2 = engine.update(370.0f, 30.0f, 55.0f, 0.8f, 0.1f);
    BatteryState state2 = engine.get_current_state();

    EXPECT_GT(state2.entropy_index, state1.entropy_index);
    EXPECT_TRUE(advisory2.require_derating); // temperature is near 60°C maximum limit
}

TEST(EnergyRuntimeTest, HighLevelRuntimeRunnerCycle) {
    AILEEEnergyRuntime runtime;

    // Default run_cycle check (with realistic small update interval of 0.1s)
    BatteryAdvisory advisory = runtime.run_cycle(390.0f, 10.0f, 26.0f, 0.95f, 0.1f);

    // Highly stable environment, should have high confidence and high torque scaling
    EXPECT_GE(advisory.confidence, 0.8f);
    EXPECT_GT(advisory.torque_scale, 0.8f);
}
