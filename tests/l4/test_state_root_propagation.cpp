#include "l4/StateRootPropagation.h"
#include "l4/ClusterTypes.h"
#include "l4/MeshAnchor.h"
#include "l4/ClusterSim.h"
#include <gtest/gtest.h>
#include <vector>
#include <cstring>

using namespace ailee::l4;

TEST(StateRootPropagationTest, AllNodesConsistent) {
    ClusterView view = {};
    view.total_nodes = 3;

    for (int i = 0; i < 3; ++i) {
        ClusterNodeState node = {};
        node.node_id_hash = 100 + i;
        node.last_envelope.context.l1_height = 10;
        for (int j = 0; j < 32; ++j) {
            node.last_envelope.context.state_root_hash[j] = 0xAA;
        }
        view.nodes.push_back(node);
    }

    MeshEpoch epoch = build_mesh_epoch(view);
    for (int j = 0; j < 32; ++j) epoch.mesh_state_root[j] = 0xAA; // explicitly override mesh_state_root
    MeshAnchor anchor = build_mesh_anchor(epoch, view);

    auto announcements = build_state_root_announcements(view);
    EXPECT_EQ(announcements.size(), 3);

    auto validation_results = validate_state_roots(view, anchor, announcements);
    EXPECT_EQ(validation_results.size(), 3);

    for (size_t i = 0; i < validation_results.size(); ++i) {
        EXPECT_TRUE(validation_results[i].accepted);
        EXPECT_FALSE(validation_results[i].rejected);
        EXPECT_EQ(validation_results[i].reason_code, 0);
    }

    for (auto& node : view.nodes) {
        for (const auto& res : validation_results) {
            if (res.node_id_hash == node.node_id_hash) {
                if (res.accepted) node.state_root_status = StateRootStatus::CONSISTENT;
                else node.state_root_status = StateRootStatus::INCONSISTENT;
            }
        }
    }

    ClusterCoherenceSummary summary = compute_cluster_coherence(view);
    EXPECT_EQ(summary.consistent_state_root_nodes, 3);
    EXPECT_EQ(summary.inconsistent_state_root_nodes, 0);
}

TEST(StateRootPropagationTest, SingleDivergentNode) {
    ClusterView view = {};
    view.total_nodes = 3;

    for (int i = 0; i < 3; ++i) {
        ClusterNodeState node = {};
        node.node_id_hash = 100 + i;
        node.last_envelope.context.l1_height = 10;
        if (i == 2) {
            for (int j = 0; j < 32; ++j) node.last_envelope.context.state_root_hash[j] = 0xBB;
        } else {
            for (int j = 0; j < 32; ++j) node.last_envelope.context.state_root_hash[j] = 0xAA;
        }
        view.nodes.push_back(node);
    }

    MeshEpoch epoch = build_mesh_epoch(view);
    for (int j = 0; j < 32; ++j) epoch.mesh_state_root[j] = 0xAA;
    MeshAnchor anchor = build_mesh_anchor(epoch, view);

    auto announcements = build_state_root_announcements(view);

    auto validation_results = validate_state_roots(view, anchor, announcements);

    EXPECT_TRUE(validation_results[0].accepted);
    EXPECT_TRUE(validation_results[1].accepted);
    EXPECT_TRUE(validation_results[2].rejected);
    EXPECT_EQ(validation_results[2].reason_code, 1);

    for (auto& node : view.nodes) {
        for (const auto& res : validation_results) {
            if (res.node_id_hash == node.node_id_hash) {
                if (res.accepted) node.state_root_status = StateRootStatus::CONSISTENT;
                else node.state_root_status = StateRootStatus::INCONSISTENT;
            }
        }
    }

    ClusterCoherenceSummary summary = compute_cluster_coherence(view);
    EXPECT_EQ(summary.consistent_state_root_nodes, 2);
    EXPECT_EQ(summary.inconsistent_state_root_nodes, 1);
}

TEST(StateRootPropagationTest, StaleEpoch) {
    ClusterView view = {};
    view.total_nodes = 2;

    for (int i = 0; i < 2; ++i) {
        ClusterNodeState node = {};
        node.node_id_hash = 100 + i;
        node.last_envelope.context.l1_height = (i == 1) ? 9 : 10;
        for (int j = 0; j < 32; ++j) {
            node.last_envelope.context.state_root_hash[j] = 0xAA;
        }
        view.nodes.push_back(node);
    }

    MeshEpoch epoch = build_mesh_epoch(view);
    epoch.epoch_height = 10;
    for (int j = 0; j < 32; ++j) epoch.mesh_state_root[j] = 0xAA;
    MeshAnchor anchor = build_mesh_anchor(epoch, view);

    auto announcements = build_state_root_announcements(view);
    auto validation_results = validate_state_roots(view, anchor, announcements);

    EXPECT_TRUE(validation_results[0].accepted);
    EXPECT_TRUE(validation_results[1].rejected);
    EXPECT_EQ(validation_results[1].reason_code, 2);
}

TEST(StateRootPropagationTest, UnknownSource) {
    ClusterView view = {};
    view.total_nodes = 1;

    ClusterNodeState node = {};
    node.node_id_hash = 100; // Will look for an announcement from 100, won't find it.
    node.last_envelope.context.l1_height = 10;
    for (int j = 0; j < 32; ++j) {
        node.last_envelope.context.state_root_hash[j] = 0xAA;
    }
    view.nodes.push_back(node);

    MeshEpoch epoch = build_mesh_epoch(view);
    epoch.epoch_height = 10;
    for (int j = 0; j < 32; ++j) epoch.mesh_state_root[j] = 0xAA;
    MeshAnchor anchor = build_mesh_anchor(epoch, view);

    auto announcements = build_state_root_announcements(view);
    announcements[0].source_node_id_hash = 999;

    auto validation_results = validate_state_roots(view, anchor, announcements);

    EXPECT_EQ(validation_results.size(), 1);
    EXPECT_TRUE(validation_results[0].rejected);
    EXPECT_EQ(validation_results[0].reason_code, 3);
}
