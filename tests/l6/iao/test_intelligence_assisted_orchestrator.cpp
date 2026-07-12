#include <gtest/gtest.h>
#include "l6/iao/IntelligenceAssistedOrchestrator.h"

using namespace ailee::l6;
using namespace ailee::l6::iao;
using namespace ailee::l6::auditor;
using namespace ailee::l6::erl;

class IAOTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(IAOTest, EvaluatesGuidanceCorrectly) {
    IntelligenceAssistedOrchestrator iao;

    TemporalMetricsBuffer buffer(32);
    // Add two states to calculate drift
    TemporalEpochState state1;
    state1.epoch_id = 1;
    state1.metrics.covariance_error = 0.5;
    state1.metrics.spectral_drift = 0.1;
    buffer.push_state(state1);

    TemporalEpochState state2;
    state2.epoch_id = 2;
    state2.metrics.covariance_error = 0.6;
    state2.metrics.spectral_drift = 0.15;
    // Drift = 0.1 + 0.05 = 0.15
    buffer.push_state(state2);

    TemporalEpochState state3;
    state3.epoch_id = 3;
    state3.metrics.covariance_error = 0.7;
    state3.metrics.spectral_drift = 0.25;
    // Drift = 0.1 + 0.1 = 0.20
    buffer.push_state(state3);

    // Sum drift = 0.15 + 0.20 = 0.35
    // Stability = 1.0 - (0.35 / 32) = 1.0 - 0.0109375 = 0.9890625

    ZkEpochValiditySurface surface;
    surface.anchor.anchor_drift = 800.0; // risk = 800 / 1000 = 0.8 -> HIGH_REPLICATION
    surface.metrics.context_leakage = 0.05; // temporal leakage = 1.0 * 0.05

    EnergyAdaptationDecision energy_dec;
    energy_dec.surface.energy_predictive_score = -0.5; // predicted energy trend = 1.0 * -0.5 -> INCREASE_BACKOFF

    auto report = iao.evaluate(buffer, surface, energy_dec);

    EXPECT_NEAR(report.long_term_stability_score, 0.9890625, 1e-6);
    // drift_forecast = (0.65 * 0.20) + (0.35 * 0.15) = 0.13 + 0.0525 = 0.1825
    EXPECT_NEAR(report.drift_accumulation_forecast, 0.1825, 1e-6);
    EXPECT_NEAR(report.predicted_energy_trend, -0.5, 1e-6);
    EXPECT_NEAR(report.anchor_chain_risk, 0.8, 1e-6);
    EXPECT_NEAR(report.temporal_leakage_projection, 0.05, 1e-6);

    EXPECT_EQ(report.recommended_scheduling_mode, "AGGRESSIVE_SCHEDULING");
    EXPECT_EQ(report.recommended_replication_mode, "HIGH_REPLICATION");
    EXPECT_EQ(report.recommended_backoff_strategy, "INCREASE_BACKOFF");
}

TEST_F(IAOTest, HandlesEmptyBuffer) {
    IntelligenceAssistedOrchestrator iao;
    TemporalMetricsBuffer buffer(32);
    ZkEpochValiditySurface surface;
    surface.anchor.anchor_drift = 0;
    surface.metrics.context_leakage = 0;
    EnergyAdaptationDecision energy_dec;
    energy_dec.surface.energy_predictive_score = 0;

    auto report = iao.evaluate(buffer, surface, energy_dec);
    EXPECT_EQ(report.long_term_stability_score, 1.0);
    EXPECT_EQ(report.drift_accumulation_forecast, 0.0);
}
