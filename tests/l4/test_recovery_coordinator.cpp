#include <gtest/gtest.h>
#include "l4/RecoveryCoordinator.h"
#include "l4/ClusterTypes.h"
#include "l3/PeerSync.h"
#include <cstring>
#include <vector>

using namespace ailee;
using namespace ailee::l4;
using namespace ailee::l3;

class RecoveryCoordinatorTest : public ::testing::Test {
public:
    void SetUp() override {}

    ClusterNodeState create_node(uint64_t node_id, SyncStatus sync_status, uint64_t local_epoch, uint64_t remote_epoch, bool set_zero_root = false) {
        ClusterNodeState node = {};
        node.node_id_hash = node_id;

        // Mock execution envelope state root
        node.last_envelope.context.l1_height = local_epoch;

        if (set_zero_root) {
            std::memset(node.last_envelope.context.state_root_hash, 0, 32);
        } else {
            for (int i = 0; i < 32; ++i) {
                node.last_envelope.context.state_root_hash[i] = (i + node_id) % 256;
            }
        }

        PeerSyncState sync = {};
        std::memset(&sync, 0, sizeof(sync));
        sync.sync_status = sync_status;
        sync.local_envelope.context.l1_height = local_epoch;
        sync.remote_envelope.remote_summary.epoch_height = remote_epoch;
        node.peer_sync_states.push_back(sync);

        return node;
    }
};

TEST_F(RecoveryCoordinatorTest, BuildRequests_StateMismatch) {
    ClusterView view = {};

    // A node that needs recovery
    ClusterNodeState needy = create_node(10, SyncStatus::NEEDS_RECOVERY, 100, 100);

    // An IN_SYNC node (makes it valid to request)
    ClusterNodeState helper = create_node(20, SyncStatus::IN_SYNC, 100, 100);

    // The needy node must see an IN_SYNC peer in its sync state to ask for recovery!
    // So we add another sync state to needy showing it sees helper.
    PeerSyncState helper_sync = {};
    std::memset(&helper_sync, 0, sizeof(helper_sync));
    helper_sync.sync_status = SyncStatus::IN_SYNC;
    needy.peer_sync_states.insert(needy.peer_sync_states.begin(), helper_sync);

    view.nodes.push_back(needy);
    view.nodes.push_back(helper);
    view.total_nodes = 2;

    auto reqs = build_recovery_requests(view);

    ASSERT_EQ(reqs.size(), 1);
    EXPECT_EQ(reqs[0].requester_node_id_hash, 10);
    EXPECT_EQ(reqs[0].target_epoch, 100);
    EXPECT_EQ(reqs[0].reason, RecoveryReason::STATE_MISMATCH);
}

TEST_F(RecoveryCoordinatorTest, BuildRequests_Behind) {
    ClusterView view = {};

    ClusterNodeState behind = create_node(10, SyncStatus::BEHIND, 100, 108); // >= 8 threshold

    PeerSyncState helper_sync = {};
    std::memset(&helper_sync, 0, sizeof(helper_sync));
    helper_sync.sync_status = SyncStatus::AHEAD; // Valid peer to ask
    behind.peer_sync_states.insert(behind.peer_sync_states.begin(), helper_sync);

    view.nodes.push_back(behind);

    auto reqs = build_recovery_requests(view);

    ASSERT_EQ(reqs.size(), 1);
    EXPECT_EQ(reqs[0].reason, RecoveryReason::BEHIND);

    // Reset behind but delta < 8
    behind = create_node(10, SyncStatus::BEHIND, 100, 107);
    behind.peer_sync_states.insert(behind.peer_sync_states.begin(), helper_sync);
    view.nodes.clear();
    view.nodes.push_back(behind);

    reqs = build_recovery_requests(view);
    EXPECT_EQ(reqs.size(), 0);
}

TEST_F(RecoveryCoordinatorTest, ProviderSelection_TieBreaking) {
    ClusterView view = {};

    RecoveryRequest req = {};
    req.requester_node_id_hash = 99;
    req.target_epoch = 100;

    // Provider 1: Valid
    ClusterNodeState p1 = create_node(10, SyncStatus::IN_SYNC, 100, 100);

    // Provider 2: Valid, smaller node_id_hash
    ClusterNodeState p2 = create_node(5, SyncStatus::IN_SYNC, 100, 100);

    // Provider 3: Valid, same node id as p2, but smaller state_root (mocked via node_id + i)
    // Wait, node_id_hash must be unique but for testing we can simulate same hash.
    ClusterNodeState p3 = create_node(5, SyncStatus::IN_SYNC, 100, 100);
    // Force a smaller state root for p3
    std::memset(p3.last_envelope.context.state_root_hash, 0x1, 32);
    std::memset(p2.last_envelope.context.state_root_hash, 0x2, 32);

    view.nodes.push_back(p1);
    view.nodes.push_back(p2);
    view.nodes.push_back(p3);

    std::vector<RecoveryRequest> reqs = {req};
    auto resps = match_recovery_responses(view, reqs);

    ASSERT_EQ(resps.size(), 1);
    EXPECT_EQ(resps[0].provider_node_id_hash, 5);

    // It should pick p3 because of tie-breaking
    EXPECT_EQ(std::memcmp(resps[0].state_root, p3.last_envelope.context.state_root_hash, 32), 0);
}

TEST_F(RecoveryCoordinatorTest, ApplyRecovery) {
    ClusterNodeState node = {};
    node.step_counter = 5;

    RecoveryResponse resp = {};
    resp.epoch = 200;
    std::memset(resp.state_root, 0xAB, 32);
    resp.full_recovery = true;

    // Setup a dummy peer sync state
    node.peer_sync_states.push_back(PeerSyncState{});

    apply_recovery(node, resp);

    EXPECT_EQ(node.engine_state.epoch.l1_height, 200);
    EXPECT_EQ(node.engine_state.state_root.root_hash[0], 0xAB);
    EXPECT_EQ(node.engine_state.context.l1_height, 200);
    EXPECT_EQ(node.last_envelope.context.l1_height, 200);
    EXPECT_EQ(node.last_envelope.context.state_root_hash[0], 0xAB);
    EXPECT_TRUE(node.peer_sync_states.empty());
    EXPECT_EQ(node.step_counter, 6);
}
