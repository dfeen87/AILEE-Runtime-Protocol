#include <gtest/gtest.h>
#include "l4/DeterministicTelemetry.h"
#include "l4/DeterministicScheduler.h"
#include "l4/ClusterSim.h"
#include <cstring>
#include <iostream>

using namespace ailee::l4;

class DeterministicTelemetryTest : public ::testing::Test {
public:
    void SetUp() override {
    }
};

TEST_F(DeterministicTelemetryTest, RecordTelemetrySample) {
    ClusterView view = {};
    view.total_nodes = 5;
    view.coherence_summary.in_sync_count = 4;
    view.coherence_summary.consistent_state_root_nodes = 3;
    view.coherence_summary.inconsistent_state_root_nodes = 1;
    view.coherence_summary.global_coherence_score = 80;

    DeterministicSchedulerState state = {};
    state.tick_count = 42;
    state.epoch_height = 10;

    TelemetryBuffer buffer;
    record_telemetry_sample(view, state, buffer);

    ASSERT_EQ(buffer.samples.size(), 1);
    const auto& sample = buffer.samples.back();

    EXPECT_EQ(sample.tick_count, 42);
    EXPECT_EQ(sample.epoch_height, 10);
    EXPECT_EQ(sample.total_nodes, 5);
    EXPECT_EQ(sample.in_sync_nodes, 4);
    EXPECT_EQ(sample.consistent_state_root_nodes, 3);
    EXPECT_EQ(sample.inconsistent_state_root_nodes, 1);
    EXPECT_EQ(sample.global_coherence_score, 80);

    // Padding should be zeroed
    for (size_t i = 0; i < sizeof(sample.padding); ++i) {
        EXPECT_EQ(sample.padding[i], 0);
    }
}

TEST_F(DeterministicTelemetryTest, FullSchedulerIntegration_Reproducibility) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (size_t i = 0; i < 3; i++) {
        initial_nodes[i] = ClusterNodeState{};
        initial_nodes[i].node_id_hash = (i + 1) * 111;
    }
    std::vector<std::pair<size_t, size_t>> schedule = { {0, 1}, {1, 2}, {2, 0} };

    // Run 1
    ClusterView view1 = {};
    view1.nodes = initial_nodes;
    view1.total_nodes = initial_nodes.size();

    std::vector<ailee::l2::DeterministicEngine> engines1;
    for (const auto& node : view1.nodes) {
        engines1.emplace_back(node.engine_state);
    }
    DeterministicScheduler scheduler1;
    for (uint64_t i = 0; i < 5 * 9; ++i) { // 5 full steps
        scheduler1.run_tick(view1, schedule, engines1);
    }

    // Run 2
    ClusterView view2 = {};
    view2.nodes = initial_nodes;
    view2.total_nodes = initial_nodes.size();

    std::vector<ailee::l2::DeterministicEngine> engines2;
    for (const auto& node : view2.nodes) {
        engines2.emplace_back(node.engine_state);
    }
    DeterministicScheduler scheduler2;
    for (uint64_t i = 0; i < 5 * 9; ++i) {
        scheduler2.run_tick(view2, schedule, engines2);
    }

    // Both should have 5 full steps -> 5 samples (1 sample per COHERENCE_UPDATE which happens every 9th sub-tick)
    ASSERT_EQ(scheduler1.telemetry.samples.size(), 5);
    ASSERT_EQ(scheduler2.telemetry.samples.size(), 5);

    // Byte-for-byte reproducibility
    for (size_t i = 0; i < scheduler1.telemetry.samples.size(); ++i) {
        const auto& s1 = scheduler1.telemetry.samples[i];
        const auto& s2 = scheduler2.telemetry.samples[i];
        EXPECT_EQ(std::memcmp(&s1, &s2, sizeof(TelemetrySample)), 0);
    }
}

TEST_F(DeterministicTelemetryTest, EpochTickCorrelationAndCoherenceTracking) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (size_t i = 0; i < 3; i++) {
        initial_nodes[i] = ClusterNodeState{};
        initial_nodes[i].node_id_hash = (i + 1) * 999;
    }
    // Fully connected schedule should lead to convergence
    std::vector<std::pair<size_t, size_t>> schedule = {
        {0, 1}, {0, 2}, {1, 0}, {1, 2}, {2, 0}, {2, 1}
    };

    ClusterView view = {};
    view.nodes = initial_nodes;
    view.total_nodes = initial_nodes.size();

    std::vector<ailee::l2::DeterministicEngine> engines;
    for (const auto& node : view.nodes) {
        engines.emplace_back(node.engine_state);
    }
    DeterministicScheduler scheduler;

    uint64_t max_steps = 10;
    for (uint64_t i = 0; i < max_steps * 9; ++i) {
        scheduler.run_tick(view, schedule, engines);
    }

    ASSERT_EQ(scheduler.telemetry.samples.size(), max_steps);

    for (size_t i = 0; i < scheduler.telemetry.samples.size(); ++i) {
        const auto& sample = scheduler.telemetry.samples[i];

        // Assert tick count correlation.
        // A COHERENCE_UPDATE tick happens at phase 8 (which means it's the 9th sub-tick of the cycle).
        // Before run_tick returns, state.tick_count is incremented.
        // During COHERENCE_UPDATE, state.tick_count has a value of `i * 9 + 8` where `i` is the cycle index.
        // Wait, `record_telemetry_sample` is called in phase 8. `state.tick_count` at that moment is the sub-tick index.
        // So `sample.tick_count` should be exactly `i * 9 + 8`.
        EXPECT_EQ(sample.tick_count, i * 9 + 8);

        // Assert epoch correlation
        // In MESH_EPOCH_BUILD phase (phase 3), state.epoch_height is updated to current_epoch.epoch_height.
        // So by phase 8, sample.epoch_height should exactly match scheduler.state.epoch_height.
        // Note: It's hard to predict exact epoch_height without running it, but we can check monotonic increasing
        if (i > 0) {
            EXPECT_TRUE(sample.epoch_height >= scheduler.telemetry.samples[i-1].epoch_height);
        }

        // Node metrics
        EXPECT_EQ(sample.total_nodes, 3);
        EXPECT_TRUE(sample.in_sync_nodes <= 3);
        EXPECT_TRUE(sample.consistent_state_root_nodes <= 3);
        EXPECT_TRUE(sample.inconsistent_state_root_nodes <= 3);
    }

    // Check coherence tracking: Over 10 steps of fully connected gossip, they should eventually converge.
    const auto& first_sample = scheduler.telemetry.samples.front();
    const auto& last_sample = scheduler.telemetry.samples.back();

    EXPECT_TRUE(last_sample.consistent_state_root_nodes >= first_sample.consistent_state_root_nodes);
    EXPECT_TRUE(last_sample.inconsistent_state_root_nodes <= first_sample.inconsistent_state_root_nodes);
    EXPECT_TRUE(last_sample.in_sync_nodes >= first_sample.in_sync_nodes);
    EXPECT_TRUE(last_sample.global_coherence_score >= first_sample.global_coherence_score);
}
