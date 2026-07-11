#include <gtest/gtest.h>
#include "l6/MeshCoherenceEngine.h"

using namespace ailee::l6;
using namespace ailee::mesh;

TEST(MeshCoherenceEngineTest, DefaultScoreIsOne) {
    MeshCoherenceEngine engine;
    MeshNodeSnapshot local{};
    EXPECT_EQ(engine.get_normalized_coherence_score(local), 1.0);
}

TEST(MeshCoherenceEngineTest, FullyCoherentPeer) {
    MeshCoherenceEngine engine;
    MeshNodeSnapshot local{};
    local.latest_l1_height = 10;
    local.latest_l2_epoch = 20;

    MeshNodeSnapshot peer = local;
    peer.node_id.id[0] = 1;

    engine.register_peer_state(peer);

    EXPECT_EQ(engine.get_normalized_coherence_score(local), 1.0);
}

TEST(MeshCoherenceEngineTest, DivergentPeer) {
    MeshCoherenceEngine engine;
    MeshNodeSnapshot local{};
    local.latest_l1_height = 10;
    local.latest_l2_epoch = 20;

    MeshNodeSnapshot peer{};
    peer.node_id.id[0] = 1;
    peer.latest_l1_height = 999;
    peer.latest_l2_epoch = 999;
    peer.latest_anchor_hash[0] = 1;
    peer.latest_state_root[0] = 1;

    engine.register_peer_state(peer);

    EXPECT_EQ(engine.get_normalized_coherence_score(local), 0.0);
}

TEST(MeshCoherenceEngineTest, PartiallyCoherentPeer) {
    MeshCoherenceEngine engine;
    MeshNodeSnapshot local{};
    local.latest_l1_height = 10;
    local.latest_l2_epoch = 20;

    MeshNodeSnapshot peer{};
    peer.node_id.id[0] = 1;
    peer.latest_l1_height = 10;
    peer.latest_l2_epoch = 999;

    engine.register_peer_state(peer);

    EXPECT_EQ(engine.get_normalized_coherence_score(local), 0.5);
}
