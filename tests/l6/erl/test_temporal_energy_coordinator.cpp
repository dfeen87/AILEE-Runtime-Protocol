#include <gtest/gtest.h>
#include "l6/erl/TemporalEnergyCoordinator.h"
#include "semantics/SemanticsTypes.h"
#include "l6/auditor/TemporalAuditor.h"

using namespace ailee::l6::erl;
using namespace ailee::semantics;
using namespace ailee::l6::auditor;
using namespace ailee::simulation::validation;

class TemporalEnergyCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.alpha_drift = 1.0;
        config_.alpha_spectral = 1.0;
        config_.alpha_memory = 1.0;
        config_.max_energy_drift = 10.0;
        config_.base_cost_factor = 2.0;
        config_.max_energy_anchor_drift = 20.0;
        config_.energy_predictive_threshold = 5.0;
        config_.energy_stability_margin = 0.5;

        coordinator_ = std::make_unique<TemporalEnergyCoordinator>(config_);
    }

    Config config_;
    std::unique_ptr<TemporalEnergyCoordinator> coordinator_;
};

TEST_F(TemporalEnergyCoordinatorTest, TestInitialEvaluation) {
    HiceMetrics metrics;
    metrics.spectral_drift = 0.5;
    metrics.delta_memory = 0.1;

    ZkEpochValiditySurface validity;
    validity.coherence.drift = 1.0;
    validity.coherence.leakage = 0.5;
    validity.anchor.anchor_drift = 2.0;

    auto decision = coordinator_->evaluate(metrics, validity);

    // E(t) = 1.0 * 1.0 + 1.0 * 0.5 + 1.0 * 0.1 = 1.6
    // Since it's the first evaluation, energy_drift = 0.0
    EXPECT_DOUBLE_EQ(decision.surface.energy_drift, 0.0);
    EXPECT_DOUBLE_EQ(decision.surface.energy_stability, 1.0); // 1.0 - 0/10
    EXPECT_DOUBLE_EQ(decision.surface.energy_cost_of_drift, 0.0);
    EXPECT_DOUBLE_EQ(decision.surface.energy_anchor_coherence, 0.9); // 1.0 - (0+2)/20
    EXPECT_DOUBLE_EQ(decision.surface.energy_predictive_score, 0.0);

    EXPECT_FALSE(decision.should_reduce_heavy_ops);
    EXPECT_FALSE(decision.should_delay_replication); // stability is 1.0 > 0.5
    EXPECT_FALSE(decision.should_increase_backoff);
}

TEST_F(TemporalEnergyCoordinatorTest, TestSubsequentEvaluation) {
    // Epoch 1
    HiceMetrics metrics1;
    metrics1.spectral_drift = 0.5;
    metrics1.delta_memory = 0.1;

    ZkEpochValiditySurface validity1;
    validity1.coherence.drift = 1.0;
    validity1.coherence.leakage = 0.5;
    validity1.anchor.anchor_drift = 2.0;

    coordinator_->evaluate(metrics1, validity1); // E(1) = 1.6

    // Epoch 2 - Significant drift
    HiceMetrics metrics2;
    metrics2.spectral_drift = 4.5;
    metrics2.delta_memory = 1.1;

    ZkEpochValiditySurface validity2;
    validity2.coherence.drift = 2.0;
    validity2.coherence.leakage = 1.0;
    validity2.anchor.anchor_drift = 4.0;

    auto decision2 = coordinator_->evaluate(metrics2, validity2);
    // E(2) = 1.0 * 2.0 + 1.0 * 4.5 + 1.0 * 1.1 = 7.6
    // energy_drift = 7.6 - 1.6 = 6.0
    EXPECT_DOUBLE_EQ(decision2.surface.energy_drift, 6.0);

    // stability = 1.0 - (6.0/10.0) = 0.4
    EXPECT_DOUBLE_EQ(decision2.surface.energy_stability, 0.4);

    // cost = 2.0 * 6.0 = 12.0
    EXPECT_DOUBLE_EQ(decision2.surface.energy_cost_of_drift, 12.0);

    // anchor_coherence = 1.0 - ((6.0 + 4.0) / 20.0) = 0.5
    EXPECT_DOUBLE_EQ(decision2.surface.energy_anchor_coherence, 0.5);

    // predictive score = 6.0 * 1.0 = 6.0
    EXPECT_DOUBLE_EQ(decision2.surface.energy_predictive_score, 6.0);

    // Flags
    // predictive > threshold (6.0 > 5.0) -> true
    EXPECT_TRUE(decision2.should_reduce_heavy_ops);
    // stability < margin (0.4 < 0.5) -> true
    EXPECT_TRUE(decision2.should_delay_replication);
    // drift > max (6.0 > 10.0) -> false
    EXPECT_FALSE(decision2.should_increase_backoff);
}
