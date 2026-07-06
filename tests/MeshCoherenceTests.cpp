#include <gtest/gtest.h>
#include "MeshCoherence.h"
#include "BuildInfo.h"
#include "ConfigInfo.h"
#include "L1Reflection.h"
#include "HeartbeatLog.h"
#include "StateLog.h"
#include <cstring>

namespace ailee {
namespace mesh {
namespace {

// Mocks
class MockRocksDbHandle : public RocksDbHandle {
public:
    uint64_t l1_height = 100;
    uint8_t anchor_hash[32] = {0xAA};

    uint64_t get_latest_l1_height() const override { return l1_height; }
    void get_latest_confirmed_anchor(uint8_t out_hash[32]) const override {
        std::memcpy(out_hash, anchor_hash, 32);
    }
    bool get_raw_value(const std::string& key, rocksdb::Slice& out_slice) const override { return false; }
    bool get_raw_anchor_slice(rocksdb::Slice& out_slice) const override { return false; }
    bool get_raw_reorg_slice(rocksdb::Slice& out_slice) const override { return false; }
    bool get_raw_block_height_slice(rocksdb::Slice& out_slice) const override { return false; }
};

class MockHeartbeatLog : public HeartbeatLog {
public:
    uint64_t epoch = 50;
    uint64_t get_latest_epoch() const override { return epoch; }
};

class MockStateRootLog : public StateRootLog {
public:
    uint8_t state_root[32] = {0xBB};
    void get_latest_state_root(uint8_t out_root[32]) const override {
        std::memcpy(out_root, state_root, 32);
    }
};

TEST(MeshCoherenceTests, ComputeNodeIdDeterminism) {
    build::BuildHashInfo build1;
    std::memset(&build1, 1, sizeof(build1));
    GenesisInfo gen1;
    std::memset(&gen1, 2, sizeof(gen1));
    ConfigInfo cfg1;
    std::memset(&cfg1, 3, sizeof(cfg1));

    MeshNodeId id1 = compute_node_id(build1, gen1, cfg1);
    MeshNodeId id2 = compute_node_id(build1, gen1, cfg1);

    // Identical inputs -> identical id
    EXPECT_EQ(std::memcmp(id1.id, id2.id, 32), 0);

    build::BuildHashInfo build2;
    std::memset(&build2, 4, sizeof(build2));

    MeshNodeId id3 = compute_node_id(build2, gen1, cfg1);

    // Different inputs -> different id
    EXPECT_TRUE(std::memcmp(id1.id, id3.id, 32) != 0);
}

TEST(MeshCoherenceTests, BuildLocalSnapshot) {
    MockRocksDbHandle rocks;
    MockHeartbeatLog heartbeat;
    MockStateRootLog stateroot;

    MeshNodeId self_id;
    std::memset(self_id.id, 0xCC, 32);

    MeshNodeSnapshot snapshot = build_local_snapshot(self_id, rocks, heartbeat, stateroot);

    EXPECT_EQ(std::memcmp(snapshot.node_id.id, self_id.id, 32), 0);
    EXPECT_EQ(snapshot.latest_l1_height, 100);
    EXPECT_EQ(snapshot.latest_l2_epoch, 50);

    uint8_t expected_anchor[32] = {0xAA};
    uint8_t expected_root[32] = {0xBB};
    EXPECT_EQ(std::memcmp(snapshot.latest_anchor_hash, expected_anchor, 32), 0);
    EXPECT_EQ(std::memcmp(snapshot.latest_state_root, expected_root, 32), 0);
}

TEST(MeshCoherenceTests, ComputeMeshCoherenceScore4) {
    MeshNodeSnapshot s1, s2;
    std::memset(&s1, 0, sizeof(s1));
    std::memset(&s2, 0, sizeof(s2));

    s1.latest_l1_height = 100;
    s2.latest_l1_height = 100;

    std::memset(s1.latest_anchor_hash, 0x11, 32);
    std::memset(s2.latest_anchor_hash, 0x11, 32);

    s1.latest_l2_epoch = 200;
    s2.latest_l2_epoch = 200;

    std::memset(s1.latest_state_root, 0x22, 32);
    std::memset(s2.latest_state_root, 0x22, 32);

    MeshCoherenceResult result = compute_mesh_coherence(s1, s2);
    EXPECT_EQ(result.score, 4);
    EXPECT_TRUE(result.l1_height_match);
    EXPECT_TRUE(result.anchor_match);
    EXPECT_TRUE(result.epoch_match);
    EXPECT_TRUE(result.state_root_match);
}

TEST(MeshCoherenceTests, ComputeMeshCoherenceScore3) {
    MeshNodeSnapshot s1, s2;
    std::memset(&s1, 0, sizeof(s1));
    std::memset(&s2, 0, sizeof(s2));

    s1.latest_l1_height = 100;
    s2.latest_l1_height = 101; // difference

    std::memset(s1.latest_anchor_hash, 0x11, 32);
    std::memset(s2.latest_anchor_hash, 0x11, 32);

    s1.latest_l2_epoch = 200;
    s2.latest_l2_epoch = 200;

    std::memset(s1.latest_state_root, 0x22, 32);
    std::memset(s2.latest_state_root, 0x22, 32);

    MeshCoherenceResult result = compute_mesh_coherence(s1, s2);
    EXPECT_EQ(result.score, 3);
    EXPECT_FALSE(result.l1_height_match);
    EXPECT_TRUE(result.anchor_match);
    EXPECT_TRUE(result.epoch_match);
    EXPECT_TRUE(result.state_root_match);
}

TEST(MeshCoherenceTests, ComputeMeshCoherenceScore0) {
    MeshNodeSnapshot s1, s2;
    std::memset(&s1, 0, sizeof(s1));
    std::memset(&s2, 0, sizeof(s2));

    s1.latest_l1_height = 100;
    s2.latest_l1_height = 101;

    std::memset(s1.latest_anchor_hash, 0x11, 32);
    std::memset(s2.latest_anchor_hash, 0x12, 32);

    s1.latest_l2_epoch = 200;
    s2.latest_l2_epoch = 201;

    std::memset(s1.latest_state_root, 0x22, 32);
    std::memset(s2.latest_state_root, 0x23, 32);

    MeshCoherenceResult result = compute_mesh_coherence(s1, s2);
    EXPECT_EQ(result.score, 0);
    EXPECT_FALSE(result.l1_height_match);
    EXPECT_FALSE(result.anchor_match);
    EXPECT_FALSE(result.epoch_match);
    EXPECT_FALSE(result.state_root_match);
}

TEST(MeshCoherenceTests, SummarizeMeshCoherence) {
    MeshNodeSnapshot self;
    std::memset(&self, 0, sizeof(self));
    self.latest_l1_height = 100;
    self.latest_l2_epoch = 200;
    std::memset(self.latest_anchor_hash, 0x11, 32);
    std::memset(self.latest_state_root, 0x22, 32);

    MeshNodeSnapshot others[3];
    std::memset(&others, 0, sizeof(others));

    // other 0: identical (score 4)
    others[0] = self;

    // other 1: 1 field different (score 3)
    others[1] = self;
    others[1].latest_l1_height = 101;

    // other 2: all fields different (score 0)
    others[2].latest_l1_height = 101;
    others[2].latest_l2_epoch = 201;
    std::memset(others[2].latest_anchor_hash, 0x12, 32);
    std::memset(others[2].latest_state_root, 0x23, 32);

    MeshCoherenceSummary summary = summarize_mesh_coherence(self, others, 3);
    EXPECT_EQ(summary.total_nodes, 3);
    EXPECT_EQ(summary.fully_coherent_nodes, 1);
    EXPECT_EQ(summary.partially_coherent_nodes, 1);
    EXPECT_EQ(summary.divergent_nodes, 1);
}

} // namespace
} // namespace mesh
} // namespace ailee
