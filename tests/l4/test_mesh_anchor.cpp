#include <gtest/gtest.h>
#include "l4/MeshAnchor.h"
#include "l4/ClusterTypes.h"
#include <cstring>
#include <openssl/sha.h>
#include <vector>

using namespace ailee::l4;

class MeshAnchorTest : public ::testing::Test {
protected:
    ClusterView create_mock_view(size_t node_count) {
        ClusterView view = {};
        view.total_nodes = node_count;
        view.nodes.resize(node_count);
        for (size_t i = 0; i < node_count; ++i) {
            // Assign some distinct determinism to node_id_hash
            view.nodes[i].node_id_hash = 1000 + i;
            view.nodes[i].last_envelope.context.l1_height = 42;

            // Populate state root with some distinct pattern
            for (int j = 0; j < 32; ++j) {
                view.nodes[i].last_envelope.context.state_root_hash[j] = static_cast<uint8_t>((i * 31 + j) % 256);
            }

            // Make some IN_SYNC for coherence
            if (i < node_count * 0.7) {
                ailee::l3::PeerSyncState sync_state = {};
                sync_state.sync_status = ailee::l3::SyncStatus::IN_SYNC;
                view.nodes[i].peer_sync_states.push_back(sync_state);
            }
        }
        return view;
    }
};

TEST_F(MeshAnchorTest, BuildMeshEpochDeterminism) {
    ClusterView view1 = create_mock_view(5);
    ClusterView view2 = create_mock_view(5);

    // Shuffle view2 nodes to ensure deterministic sorting inside build_mesh_epoch
    std::swap(view2.nodes[0], view2.nodes[4]);
    std::swap(view2.nodes[1], view2.nodes[3]);

    MeshEpoch epoch1 = build_mesh_epoch(view1);
    MeshEpoch epoch2 = build_mesh_epoch(view2);

    EXPECT_EQ(epoch1.epoch_height, 42);
    EXPECT_EQ(epoch2.epoch_height, 42);

    // Hashes should be exactly identical
    EXPECT_EQ(std::memcmp(epoch1.epoch_hash, epoch2.epoch_hash, 32), 0);
    EXPECT_EQ(std::memcmp(epoch1.mesh_state_root, epoch2.mesh_state_root, 32), 0);

    // Ensure it's not all zeros
    bool all_zeros = true;
    for (int i = 0; i < 32; ++i) {
        if (epoch1.epoch_hash[i] != 0) all_zeros = false;
    }
    EXPECT_FALSE(all_zeros);
}

TEST_F(MeshAnchorTest, BuildMeshAnchorDeterminism) {
    ClusterView view = create_mock_view(10);
    MeshEpoch epoch = build_mesh_epoch(view);

    MeshAnchor anchor1 = build_mesh_anchor(epoch, view);
    MeshAnchor anchor2 = build_mesh_anchor(epoch, view);

    EXPECT_EQ(anchor1.total_nodes, 10);
    EXPECT_EQ(anchor1.in_sync_nodes, 7); // 70% of 10
    EXPECT_EQ(anchor1.anchor_id, anchor2.anchor_id);

    EXPECT_EQ(std::memcmp(anchor1.anchor_hash, anchor2.anchor_hash, 32), 0);

    // Check derivation logic
    uint64_t expected_anchor_id = 0;
    for (int i = 0; i < 8; ++i) {
        expected_anchor_id |= (static_cast<uint64_t>(epoch.epoch_hash[i]) << (i * 8));
    }
    EXPECT_EQ(anchor1.anchor_id, expected_anchor_id);
}

TEST_F(MeshAnchorTest, BuildMeshPropagationEnvelopeDeterminism) {
    ClusterView view = create_mock_view(3);
    MeshEpoch epoch = build_mesh_epoch(view);
    MeshAnchor anchor = build_mesh_anchor(epoch, view);

    uint64_t source_id = 12345;
    MeshPropagationEnvelope env1 = build_mesh_propagation_envelope(source_id, anchor);
    MeshPropagationEnvelope env2 = build_mesh_propagation_envelope(source_id, anchor);

    EXPECT_EQ(env1.source_node_id_hash, source_id);
    EXPECT_EQ(std::memcmp(env1.propagation_hash, env2.propagation_hash, 32), 0);

    bool all_zeros = true;
    for (int i = 0; i < 32; ++i) {
        if (env1.propagation_hash[i] != 0) all_zeros = false;
    }
    EXPECT_FALSE(all_zeros);
}

TEST_F(MeshAnchorTest, ComputeMeshCoherenceScore) {
    MeshAnchor anchor = {};
    anchor.total_nodes = 10;
    anchor.in_sync_nodes = 7;

    uint64_t score = compute_mesh_coherence_score(anchor);
    EXPECT_EQ(score, 70);

    anchor.in_sync_nodes = 10;
    score = compute_mesh_coherence_score(anchor);
    EXPECT_EQ(score, 100);

    anchor.total_nodes = 0;
    anchor.in_sync_nodes = 0;
    score = compute_mesh_coherence_score(anchor);
    EXPECT_EQ(score, 0);
}
