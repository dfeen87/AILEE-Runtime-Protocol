#include <gtest/gtest.h>
#include "l3/PeerSync.h"
#include <cstring>

using namespace ailee;
using namespace ailee::l3;
using namespace ailee::l2;

class PeerSyncTest : public ::testing::Test {
public:
    ExecutionEnvelope local_env;
    GossipEnvelope remote_env;

    void SetUp() override {
        std::memset(&local_env, 0, sizeof(local_env));
        std::memset(&remote_env, 0, sizeof(remote_env));

        // Default to same height and matching state root
        local_env.context.l1_height = 100;
        remote_env.remote_summary.epoch_height = 100;

        for(int i=0; i<32; i++){
            local_env.context.state_root_hash[i] = i;
            remote_env.remote_summary.state_root_hash[i] = i;
        }

        local_env.context.mesh_coherence_score = 4;
        remote_env.remote_summary.coherence_score = 4;
    }
};

TEST_F(PeerSyncTest, InSyncStatus) {
    PeerSyncState state = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state.sync_status, SyncStatus::IN_SYNC);
    EXPECT_EQ(state.coherence_delta, 0);

    PeerSyncSummary summary = summarize_peer_sync(state);
    EXPECT_EQ(summary.sync_status, static_cast<uint8_t>(SyncStatus::IN_SYNC));
    EXPECT_EQ(summary.delta, 0);
}

TEST_F(PeerSyncTest, NeedsRecoveryStatus) {
    // Change remote state root
    remote_env.remote_summary.state_root_hash[0] = 99;

    PeerSyncState state = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state.sync_status, SyncStatus::NEEDS_RECOVERY);
}

TEST_F(PeerSyncTest, AheadStatus) {
    local_env.context.l1_height = 105;

    PeerSyncState state = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state.sync_status, SyncStatus::AHEAD);
}

TEST_F(PeerSyncTest, BehindStatus) {
    local_env.context.l1_height = 95;

    PeerSyncState state = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state.sync_status, SyncStatus::BEHIND);
}

TEST_F(PeerSyncTest, StaleStatus) {
    // remote way behind
    local_env.context.l1_height = 150;
    remote_env.remote_summary.epoch_height = 100;

    PeerSyncState state1 = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state1.sync_status, SyncStatus::STALE);

    // local way behind
    local_env.context.l1_height = 100;
    remote_env.remote_summary.epoch_height = 150;

    PeerSyncState state2 = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state2.sync_status, SyncStatus::STALE);
}

TEST_F(PeerSyncTest, CoherenceDelta) {
    local_env.context.mesh_coherence_score = 4;
    remote_env.remote_summary.coherence_score = 2;

    PeerSyncState state = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state.coherence_delta, 2);

    local_env.context.mesh_coherence_score = 1;
    remote_env.remote_summary.coherence_score = 3;

    PeerSyncState state2 = compute_peer_sync(local_env, remote_env);
    EXPECT_EQ(state2.coherence_delta, -2);
}
