#include "NodeIdentity.h"
#include "BuildInfo.h"
#include "L1Reflection.h" // contains GenesisInfo
#include "ConfigInfo.h"

#include <gtest/gtest.h>
#include <cstring>

using namespace ailee;
using namespace ailee::identity;

class NodeIdentityTest : public testing::Test {
public:
    void SetUp() override {
        // Initialize default valid inputs
        std::memset(build.build_hash, 0x11, 32);
        build.build_number = 100;

        std::memset(genesis.genesis_anchor_root, 0x22, 32);

        std::memset(config.config_hash, 0x33, 32);
        config.protocol_version = 1;
    }

    BuildInfo build;
    GenesisInfo genesis;
    ConfigInfo config;
};

TEST_F(NodeIdentityTest, DeterminismSameInputs) {
    NodeId id1 = compute_node_id(build, genesis, config);
    NodeId id2 = compute_node_id(build, genesis, config);

    EXPECT_TRUE(node_id_equal(id1, id2));
    EXPECT_EQ(node_id_compare_lex(id1, id2), 0);
}

TEST_F(NodeIdentityTest, DeterminismDifferentBuildHash) {
    NodeId id1 = compute_node_id(build, genesis, config);

    BuildInfo changed_build = build;
    changed_build.build_hash[0] = 0x99;
    NodeId id2 = compute_node_id(changed_build, genesis, config);

    EXPECT_FALSE(node_id_equal(id1, id2));
    EXPECT_NE(node_id_compare_lex(id1, id2), 0);
}

TEST_F(NodeIdentityTest, DeterminismDifferentBuildNumber) {
    NodeId id1 = compute_node_id(build, genesis, config);

    BuildInfo changed_build = build;
    changed_build.build_number = 101;
    NodeId id2 = compute_node_id(changed_build, genesis, config);

    EXPECT_FALSE(node_id_equal(id1, id2));
    EXPECT_NE(node_id_compare_lex(id1, id2), 0);
}

TEST_F(NodeIdentityTest, DeterminismDifferentGenesisRoot) {
    NodeId id1 = compute_node_id(build, genesis, config);

    GenesisInfo changed_genesis = genesis;
    changed_genesis.genesis_anchor_root[31] = 0x99;
    NodeId id2 = compute_node_id(build, changed_genesis, config);

    EXPECT_FALSE(node_id_equal(id1, id2));
    EXPECT_NE(node_id_compare_lex(id1, id2), 0);
}

TEST_F(NodeIdentityTest, DeterminismDifferentConfigHash) {
    NodeId id1 = compute_node_id(build, genesis, config);

    ConfigInfo changed_config = config;
    changed_config.config_hash[15] = 0x99;
    NodeId id2 = compute_node_id(build, genesis, changed_config);

    EXPECT_FALSE(node_id_equal(id1, id2));
    EXPECT_NE(node_id_compare_lex(id1, id2), 0);
}

TEST_F(NodeIdentityTest, DeterminismDifferentProtocolVersion) {
    NodeId id1 = compute_node_id(build, genesis, config);

    ConfigInfo changed_config = config;
    changed_config.protocol_version = 2;
    NodeId id2 = compute_node_id(build, genesis, changed_config);

    EXPECT_FALSE(node_id_equal(id1, id2));
    EXPECT_NE(node_id_compare_lex(id1, id2), 0);
}

TEST_F(NodeIdentityTest, CompareLexicographicalOrdering) {
    NodeId id1;
    std::memset(id1.id, 0x00, 32);

    NodeId id2;
    std::memset(id2.id, 0x00, 32);

    EXPECT_EQ(node_id_compare_lex(id1, id2), 0);

    id2.id[31] = 0x01;
    EXPECT_LT(node_id_compare_lex(id1, id2), 0);
    EXPECT_GT(node_id_compare_lex(id2, id1), 0);

    id1.id[0] = 0xFF;
    EXPECT_GT(node_id_compare_lex(id1, id2), 0);
    EXPECT_LT(node_id_compare_lex(id2, id1), 0);
}

TEST_F(NodeIdentityTest, ComputeForMeshWrapper) {
    NodeId id1 = compute_node_id(build, genesis, config);
    NodeId id2 = compute_node_id_for_mesh(build, genesis, config);

    EXPECT_TRUE(node_id_equal(id1, id2));
}
