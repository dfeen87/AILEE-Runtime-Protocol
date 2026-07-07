#include <gtest/gtest.h>
#include "l3/GossipLayer.h"
#include <cstring>

using namespace ailee;
using namespace ailee::l3;
using namespace ailee::l2;
using namespace ailee::mesh;

class GossipLayerTest : public ::testing::Test {
public:
    ExecutionEnvelope envelope;
    MeshCoherenceResult coherence;

    void SetUp() override {
        std::memset(&envelope, 0, sizeof(envelope));
        std::memset(&coherence, 0, sizeof(coherence));

        // Setup some arbitrary but deterministic data
        for (int i = 0; i < 32; ++i) {
            envelope.context.node_identity_hash[i] = i;
            envelope.context.state_root_hash[i] = 32 - i;
            envelope.context.epoch_hash[i] = i % 16;
        }
        envelope.context.l1_height = 100;
        coherence.score = 4;
        envelope.context.mesh_coherence_score = coherence.score;
    }
};

TEST_F(GossipLayerTest, BuildGossipMessageDeterminism) {
    uint64_t seq = 10;
    GossipMessage msg1 = build_gossip_message(envelope, coherence, seq);
    GossipMessage msg2 = build_gossip_message(envelope, coherence, seq);

    EXPECT_EQ(msg1.sequence_number, 11);
    EXPECT_EQ(msg1.summary.epoch_height, 100);
    EXPECT_EQ(msg1.summary.coherence_score, 4);
    EXPECT_TRUE(msg1.summary.flags & GOSSIP_FLAG_HEALTHY);

    EXPECT_EQ(std::memcmp(msg1.summary.node_identity_fingerprint, envelope.context.node_identity_hash, 32), 0);
    EXPECT_EQ(std::memcmp(msg1.summary.state_root_hash, envelope.context.state_root_hash, 32), 0);
    EXPECT_EQ(std::memcmp(msg1.summary.epoch_hash, envelope.context.epoch_hash, 32), 0);

    // Hash should be identical for identical inputs
    EXPECT_EQ(std::memcmp(msg1.message_hash, msg2.message_hash, 32), 0);
}

TEST_F(GossipLayerTest, ReceiveGossipMessageNormalization) {
    uint64_t seq = 10;
    GossipMessage msg = build_gossip_message(envelope, coherence, seq);

    GossipEnvelope env1 = receive_gossip_message(msg);

    // Change sequence number, should yield different message hash but same normalized hash
    GossipMessage msg2 = build_gossip_message(envelope, coherence, seq + 5);
    GossipEnvelope env2 = receive_gossip_message(msg2);

    EXPECT_EQ(env1.received_sequence, 11);
    EXPECT_EQ(env2.received_sequence, 16);

    EXPECT_EQ(std::memcmp(env1.normalized_hash, env2.normalized_hash, 32), 0);
    EXPECT_NE(std::memcmp(msg.message_hash, msg2.message_hash, 32), 0);
}
