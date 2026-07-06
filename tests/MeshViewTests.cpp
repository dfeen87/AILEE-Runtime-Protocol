#include <gtest/gtest.h>
#include "mesh/MeshView.h"
#include "BuildInfo.h"
#include "ConfigInfo.h"
#include "L1Reflection.h"
#include "HeartbeatLog.h"
#include "StateLog.h"
#include <cstring>

namespace ailee {
namespace mesh {
namespace {

// Mock dependencies
class MockRocksDbHandle : public RocksDbHandle {
public:
    uint64_t l1_height = 100;
    uint8_t anchor_hash[32] = {0xAA};

    uint64_t get_latest_l1_height() const override {
        return l1_height;
    }
    void get_latest_confirmed_anchor(uint8_t out_hash[32]) const override {
        std::memcpy(out_hash, anchor_hash, 32);
    }
};

class MockHeartbeatLog : public HeartbeatLog {
public:
    uint64_t epoch = 50;

    uint64_t get_latest_epoch() const override {
        return epoch;
    }
};

class MockStateRootLog : public StateRootLog {
public:
    uint8_t state_root[32] = {0xBB};

    void get_latest_state_root(uint8_t out_root[32]) const override {
        std::memcpy(out_root, state_root, 32);
    }
};

TEST(MeshViewTests, ComputeNodeIdStability) {
    build::BuildHashInfo build;
    std::memset(build.build_hash, 0x01, 32);
    std::memset(build.protocol_version_hash, 0x02, 32);

    GenesisInfo genesis;
    std::memset(genesis.genesis_anchor_root, 0x03, 32);

    ConfigInfo config;
    std::memset(config.config_hash, 0x04, 32);

    MeshNodeId id1 = compute_node_id(build, genesis, config);
    MeshNodeId id2 = compute_node_id(build, genesis, config);

    // Verify determinism
    EXPECT_EQ(std::memcmp(id1.id, id2.id, 32), 0);

    // Verify output is not just zero
    uint8_t zero[32] = {0};
    EXPECT_FALSE(std::memcmp(id1.id, zero, 32) == 0);
}

TEST(MeshViewTests, BuildLocalMeshView) {
    MockRocksDbHandle rocksdb;
    MockHeartbeatLog heartbeat;
    MockStateRootLog stateroot;

    // Fill mocks with specific data
    std::memset(rocksdb.anchor_hash, 0x11, 32);
    std::memset(stateroot.state_root, 0x22, 32);
    rocksdb.l1_height = 42;
    heartbeat.epoch = 84;

    MeshView view = build_local_mesh_view(rocksdb, heartbeat, stateroot);

    EXPECT_EQ(view.latest_l1_height, 42);
    EXPECT_EQ(std::memcmp(view.latest_confirmed_anchor_hash, rocksdb.anchor_hash, 32), 0);
    EXPECT_EQ(view.latest_l2_epoch, 84);
    EXPECT_EQ(std::memcmp(view.latest_l2_state_root, stateroot.state_root, 32), 0);
    EXPECT_EQ(view.num_snapshots, 0);
}

TEST(MeshViewTests, ComputeMeshCoherence) {
    MeshView v1;
    std::memset(&v1, 0, sizeof(v1));
    v1.latest_l1_height = 100;
    std::memset(v1.latest_confirmed_anchor_hash, 0xAA, 32);
    v1.latest_l2_epoch = 50;
    std::memset(v1.latest_l2_state_root, 0xBB, 32);

    MeshView v2 = v1; // Perfect match

    EXPECT_EQ(compute_mesh_coherence(v1, v2), 4);

    v2.latest_l1_height = 101; // One mismatch
    EXPECT_EQ(compute_mesh_coherence(v1, v2), 3);

    std::memset(v2.latest_confirmed_anchor_hash, 0xAB, 32); // Two mismatches
    EXPECT_EQ(compute_mesh_coherence(v1, v2), 2);

    v2.latest_l2_epoch = 51; // Three mismatches
    EXPECT_EQ(compute_mesh_coherence(v1, v2), 1);

    std::memset(v2.latest_l2_state_root, 0xBC, 32); // Four mismatches
    EXPECT_EQ(compute_mesh_coherence(v1, v2), 0);
}

} // namespace
} // namespace mesh
} // namespace ailee
