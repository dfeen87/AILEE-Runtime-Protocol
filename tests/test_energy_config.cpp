#include "ailee/energy/energy_config.hpp"
#include <gtest/gtest.h>

#ifndef EXPECT_FLOAT_EQ
#define EXPECT_FLOAT_EQ(A, B) EXPECT_NEAR(A, B, 1e-5f)
#endif

using namespace ailee::energy;

TEST(EnergyConfigTest, DefaultInitializationAndValues) {
    EnergyConfig config;

    // Verify requested defaults
    EXPECT_FLOAT_EQ(config.min_confidence_threshold, 0.20f);
    EXPECT_FLOAT_EQ(config.grace_confidence_threshold, 0.10f);
    EXPECT_FLOAT_EQ(config.fallback_position_scale, 0.1f);
    EXPECT_EQ(config.config_version, "5.0.0-energy");

    // Other defaults
    EXPECT_FLOAT_EQ(config.nominal_capacity_ah, 75.0f);
    EXPECT_FLOAT_EQ(config.nominal_voltage_v, 400.0f);
    EXPECT_FLOAT_EQ(config.max_current_a, 500.0f);
    EXPECT_FLOAT_EQ(config.max_temperature_c, 60.0f);
    EXPECT_FLOAT_EQ(config.min_temperature_c, -20.0f);
    EXPECT_FLOAT_EQ(config.lambda, 1e-6f);
    EXPECT_FLOAT_EQ(config.entropy_weight, 0.5f);
    EXPECT_FLOAT_EQ(config.phi_decay_rate, 0.001f);

    // Fallback baseline defaults
    EXPECT_FLOAT_EQ(config.fallback_torque_scale, 0.3f);
    EXPECT_FLOAT_EQ(config.fallback_charge_current_a, 10.0f);
    EXPECT_FLOAT_EQ(config.fallback_voltage_v, 360.0f);
    EXPECT_FLOAT_EQ(config.fallback_max_safe_temperature_c, 45.0f);
    EXPECT_FLOAT_EQ(config.fallback_discharge_rate_c, 0.5f);

    // Validate default config
    EXPECT_TRUE(config.validate());
}

TEST(EnergyConfigTest, InvalidParameterValidation) {
    EnergyConfig config;

    // Invalid confidence thresholds
    config.min_confidence_threshold = 1.5f;
    EXPECT_FALSE(config.validate());
    config.min_confidence_threshold = 0.20f;

    config.grace_confidence_threshold = -0.5f;
    EXPECT_FALSE(config.validate());
    config.grace_confidence_threshold = 0.10f;

    // grace > min
    config.grace_confidence_threshold = 0.30f;
    EXPECT_FALSE(config.validate());
    config.grace_confidence_threshold = 0.10f;

    // Invalid fallback scale
    config.fallback_position_scale = 0.0f;
    EXPECT_FALSE(config.validate());
    config.fallback_position_scale = -0.1f;
    EXPECT_FALSE(config.validate());
    config.fallback_position_scale = 1.1f;
    EXPECT_FALSE(config.validate());
    config.fallback_position_scale = 0.1f;

    // Empty version
    config.config_version = "";
    EXPECT_FALSE(config.validate());
    config.config_version = "5.0.0-energy";

    // Invalid capacity
    config.nominal_capacity_ah = -10.0f;
    EXPECT_FALSE(config.validate());
    config.nominal_capacity_ah = 75.0f;

    // Invalid temperature range
    config.max_temperature_c = -30.0f;
    EXPECT_FALSE(config.validate());
}
