#include <gtest/gtest.h>
#include "l4/ClusterSim.h"
#include <cstring>
#include <iostream>

using namespace ailee::l4;
using namespace ailee::l2;
using namespace ailee::l3;

class ClusterSimTest : public ::testing::Test {
public:
    void SetUp() override {
        // Initialization if needed
    }
};

TEST_F(ClusterSimTest, DeterministicGossipPropagation) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (size_t i = 0; i < 3; i++) {
        initial_nodes[i] = ClusterNodeState{};
        initial_nodes[i].node_id_hash = i + 1; // 1, 2, 3
    }

    // A linear gossip schedule 0 -> 1 -> 2
    std::vector<std::pair<size_t, size_t>> schedule = {
        {0, 1},
        {1, 2}
    };

    ClusterView result = run_cluster_simulation(initial_nodes, schedule, 1);

    ASSERT_EQ(result.total_nodes, 3);
    ASSERT_EQ(result.total_steps, 1);

    // Node 1 should have received gossip from Node 0
    ASSERT_EQ(result.nodes[1].peer_sync_states.size(), 1);
    // Node 2 should have received gossip from Node 1
    ASSERT_EQ(result.nodes[2].peer_sync_states.size(), 1);
    // Node 0 received none
    ASSERT_EQ(result.nodes[0].peer_sync_states.size(), 0);
}

TEST_F(ClusterSimTest, DeterministicPeerSyncConvergence) {
    std::vector<ClusterNodeState> initial_nodes(4);
    for (size_t i = 0; i < 4; i++) {
        initial_nodes[i] = ClusterNodeState{};
        initial_nodes[i].node_id_hash = i + 10;
    }

    // Fully connected mesh gossip for 5 steps
    std::vector<std::pair<size_t, size_t>> schedule;
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            if (i != j) {
                schedule.push_back({i, j});
            }
        }
    }

    ClusterView result = run_cluster_simulation(initial_nodes, schedule, 5);

    // Since all start with the same deterministic inputs, their execution
    // context and state roots will be identical, so they should all converge to IN_SYNC.
    for (size_t i = 0; i < 4; i++) {
        // Since we step 5 times, and each node receives from 3 peers each step,
        // it should have 15 sync states.
        ASSERT_EQ(result.nodes[i].peer_sync_states.size(), 15);

        // Latest sync state for this node should be IN_SYNC
        const auto& latest_sync = result.nodes[i].peer_sync_states.back();
        EXPECT_EQ(latest_sync.sync_status, SyncStatus::IN_SYNC);
    }

    // Coherence should be 100
    ClusterCoherenceSummary summary = compute_cluster_coherence(result);
    EXPECT_EQ(summary.in_sync_count, 4);
    EXPECT_EQ(summary.global_coherence_score, 100);
}

TEST_F(ClusterSimTest, ClusterCoherenceCalculation) {
    ClusterView mock_view = {};
    mock_view.total_nodes = 5;
    mock_view.nodes.resize(5);

    for (size_t i = 0; i < 5; i++) {
        mock_view.nodes[i] = ClusterNodeState{};
    }

    // Node 0: IN_SYNC
    PeerSyncState s0 = {};
    s0.sync_status = SyncStatus::IN_SYNC;
    mock_view.nodes[0].peer_sync_states.push_back(s0);

    // Node 1: IN_SYNC
    PeerSyncState s1 = {};
    s1.sync_status = SyncStatus::IN_SYNC;
    mock_view.nodes[1].peer_sync_states.push_back(s1);

    // Node 2: AHEAD
    PeerSyncState s2 = {};
    s2.sync_status = SyncStatus::AHEAD;
    mock_view.nodes[2].peer_sync_states.push_back(s2);

    // Node 3: NEEDS_RECOVERY
    PeerSyncState s3 = {};
    s3.sync_status = SyncStatus::NEEDS_RECOVERY;
    mock_view.nodes[3].peer_sync_states.push_back(s3);

    // Node 4: empty sync state (should count as STALE)
    // empty peer_sync_states

    ClusterCoherenceSummary summary = compute_cluster_coherence(mock_view);

    EXPECT_EQ(summary.in_sync_count, 2);
    EXPECT_EQ(summary.ahead_count, 1);
    EXPECT_EQ(summary.behind_count, 0);
    EXPECT_EQ(summary.needs_recovery_count, 1);
    EXPECT_EQ(summary.stale_count, 1);

    // Score = (2 * 100) / 5 = 40
    EXPECT_EQ(summary.global_coherence_score, 40);
}

TEST_F(ClusterSimTest, ReproducibilityAcrossRuns) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (size_t i = 0; i < 3; i++) {
        initial_nodes[i] = ClusterNodeState{};
        initial_nodes[i].node_id_hash = i * 999;
    }

    std::vector<std::pair<size_t, size_t>> schedule = {
        {0, 1}, {1, 2}, {2, 0}
    };

    ClusterView run1 = run_cluster_simulation(initial_nodes, schedule, 10);
    ClusterView run2 = run_cluster_simulation(initial_nodes, schedule, 10);

    // We expect the final views to be bit-for-bit identical since it's deterministic
    // We can use memcmp since these are essentially PODs, but wait, std::vector
    // is inside the structs. So we need to compare the POD parts or just sizes/values.

    EXPECT_EQ(run1.total_nodes, run2.total_nodes);
    EXPECT_EQ(run1.total_steps, run2.total_steps);

    for (size_t i = 0; i < run1.nodes.size(); ++i) {
        EXPECT_EQ(run1.nodes[i].step_counter, run2.nodes[i].step_counter);
        EXPECT_EQ(run1.nodes[i].node_id_hash, run2.nodes[i].node_id_hash);

        // Memcmp the inner engine state
        EXPECT_EQ(std::memcmp(&run1.nodes[i].engine_state, &run2.nodes[i].engine_state, sizeof(EngineState)), 0);

        // Memcmp the last envelope
        EXPECT_EQ(std::memcmp(&run1.nodes[i].last_envelope, &run2.nodes[i].last_envelope, sizeof(ExecutionEnvelope)), 0);

        // Memcmp peer sync vectors (assuming identical sizes)
        ASSERT_EQ(run1.nodes[i].peer_sync_states.size(), run2.nodes[i].peer_sync_states.size());
        for (size_t j = 0; j < run1.nodes[i].peer_sync_states.size(); ++j) {
            EXPECT_EQ(std::memcmp(&run1.nodes[i].peer_sync_states[j], &run2.nodes[i].peer_sync_states[j], sizeof(PeerSyncState)), 0);
        }
    }
}
