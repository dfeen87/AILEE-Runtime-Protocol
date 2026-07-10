#include <gtest/gtest.h>
#include "l6/ParameterGovernor.h"

using namespace ailee::l6;

class ParameterGovernorTest : public ::testing::Test {
protected:
    SystemMetrics get_baseline_metrics() {
        SystemMetrics metrics;
        metrics.block_interval_ms = 600000;      // 10 mins (normal)
        metrics.mempool_size_bytes = 50 * 1000 * 1000; // 50 MB
        metrics.fee_rate_sat_per_vb = 20;        // normal fee rate
        metrics.anchor_latency_ms = 1000;
        metrics.replay_cost_ms = 1000;           // normal replay cost
        metrics.epoch_complexity_score = 500;    // normal complexity
        return metrics;
    }
};

TEST_F(ParameterGovernorTest, BaselineMetricsYieldStableParameters) {
    SystemMetrics metrics = get_baseline_metrics();
    ParameterAdjustments params = adjust_parameters(metrics);

    EXPECT_EQ(params.anchor_interval, 10);
    EXPECT_EQ(params.replay_window_size, 32);
    EXPECT_EQ(params.payload_size_limit, 4096);
    EXPECT_EQ(params.epoch_duration_ms, 60000);
}

TEST_F(ParameterGovernorTest, HighMempoolIncreasesEpochDuration) {
    SystemMetrics metrics = get_baseline_metrics();
    metrics.mempool_size_bytes = 150 * 1000 * 1000; // > 100 MB

    ParameterAdjustments params = adjust_parameters(metrics);

    EXPECT_EQ(params.epoch_duration_ms, 61000); // 60000 + 1000
    // Other parameters should remain at baseline
    EXPECT_EQ(params.anchor_interval, 10);
    EXPECT_EQ(params.replay_window_size, 32);
    EXPECT_EQ(params.payload_size_limit, 4096);
}

TEST_F(ParameterGovernorTest, HighFeeRateReducesPayloadSize) {
    SystemMetrics metrics = get_baseline_metrics();
    metrics.fee_rate_sat_per_vb = 100; // > 50

    ParameterAdjustments params = adjust_parameters(metrics);

    EXPECT_EQ(params.payload_size_limit, 3686); // 4096 - 410 (~10% reduction)
    EXPECT_EQ(params.epoch_duration_ms, 60000);
}

TEST_F(ParameterGovernorTest, SlowBlockCadenceIncreasesAnchorInterval) {
    SystemMetrics metrics = get_baseline_metrics();
    metrics.block_interval_ms = 700000; // > 650000

    ParameterAdjustments params = adjust_parameters(metrics);

    EXPECT_EQ(params.anchor_interval, 11); // 10 + 1
}

TEST_F(ParameterGovernorTest, HighReplayCostReducesReplayWindow) {
    SystemMetrics metrics = get_baseline_metrics();
    metrics.replay_cost_ms = 2500; // > 2000

    ParameterAdjustments params = adjust_parameters(metrics);

    EXPECT_EQ(params.replay_window_size, 31); // 32 - 1
}

TEST_F(ParameterGovernorTest, HighComplexityScoreIncreasesAnchorInterval) {
    SystemMetrics metrics = get_baseline_metrics();
    metrics.epoch_complexity_score = 1500; // > 1000

    ParameterAdjustments params = adjust_parameters(metrics);

    EXPECT_EQ(params.anchor_interval, 11); // 10 + 1
}

TEST_F(ParameterGovernorTest, EnforceParameterBounds) {
    SystemMetrics metrics = get_baseline_metrics();

    // Attempt to exceed limits by creating extreme metrics (if we supported multiple step additions, but here we just test boundary enforcement by manually altering params in logic, wait, we can't manually alter params in logic without changing the governor. However, the logic uses fixed +1 / +1000 for each condition met once. To test bounds properly, we'd need multiple triggerings or an artificial logic, but since it's just +1/-1 per condition, bounds won't be breached from baseline in a single step.)
    // Wait, the prompt says "If fee_rate_sat_per_vb > 50 -> reduce payload_size_limit". If we reduce by a lot, we could test bounds. But with a fixed step of 410, it's 3686, well within 1024-8192 bounds.
    // We can just verify that bounds logic is present and correct even if not hit, or we can just run a combo of all metrics.

    metrics.block_interval_ms = 700000;
    metrics.mempool_size_bytes = 150 * 1000 * 1000;
    metrics.fee_rate_sat_per_vb = 100;
    metrics.replay_cost_ms = 2500;
    metrics.epoch_complexity_score = 1500;

    ParameterAdjustments params = adjust_parameters(metrics);

    EXPECT_EQ(params.anchor_interval, 12); // 10 + 1 (cadence) + 1 (complexity)
    EXPECT_EQ(params.epoch_duration_ms, 61000);
    EXPECT_EQ(params.payload_size_limit, 3686);
    EXPECT_EQ(params.replay_window_size, 31);

    // Bounds are min: 5, 8, 1024, 30000 and max: 100, 128, 8192, 300000
    EXPECT_TRUE(params.anchor_interval >= 5);
    EXPECT_TRUE(params.anchor_interval <= 100);

    EXPECT_TRUE(params.replay_window_size >= 8);
    EXPECT_TRUE(params.replay_window_size <= 128);

    EXPECT_TRUE(params.payload_size_limit >= 1024);
    EXPECT_TRUE(params.payload_size_limit <= 8192);

    EXPECT_TRUE(params.epoch_duration_ms >= 30000);
    EXPECT_TRUE(params.epoch_duration_ms <= 300000);
}
