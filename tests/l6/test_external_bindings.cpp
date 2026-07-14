#include <gtest/gtest.h>
#include "l6/JsonBindings.h"
#include "l6/ReplayExport.h"
#include "l6/FederationExport.h"
#include "l4/ClusterTypes.h"
#include "l4/ReplayTick.h"
#include "l4/MeshAnchor.h"

using namespace ailee::l6;
using namespace ailee::l4;

TEST(ExternalBindingsTest, DeterministicJsonOrdering) {
    ExternalEnvelope env;
    env.id = 42;
    env.timestamp = 1000;
    env.payload_hash = "abcdef";

    std::string json = JsonBindings::to_json(env);

    // Keys should be id, payload_hash, timestamp in that order
    EXPECT_EQ(json, "{\"id\":42,\"payload_hash\":\"abcdef\",\"timestamp\":1000}");
}

TEST(ExternalBindingsTest, JSONRoundTrip) {
    ExternalReplayTick tick;
    tick.tick_index = 99;
    tick.scheduler_state_hash = "0xdeadbeef";
    tick.telemetry_json = "{\"test\":1}";
    tick.cluster_view.node_count = 5;
    tick.cluster_view.coherence = 95.5;

    ExternalEnvelope env1;
    env1.id = 1;
    env1.timestamp = 100;
    env1.payload_hash = "hash1";

    ExternalEnvelope env2;
    env2.id = 2;
    env2.timestamp = 200;
    env2.payload_hash = "hash2";

    tick.cluster_view.envelopes.push_back(env1);
    tick.cluster_view.envelopes.push_back(env2);

    std::string json = JsonBindings::to_json(tick);

    ExternalReplayTick parsed = JsonBindings::from_json_tick(json);

    EXPECT_EQ(parsed.tick_index, 99);
    EXPECT_EQ(parsed.scheduler_state_hash, "0xdeadbeef");
    EXPECT_EQ(parsed.telemetry_json, "{\"test\":1}");
    EXPECT_EQ(parsed.cluster_view.node_count, 5);
    EXPECT_TRUE(std::abs(parsed.cluster_view.coherence - 95.5) < 0.001);
    ASSERT_EQ(parsed.cluster_view.envelopes.size(), 2);
    EXPECT_EQ(parsed.cluster_view.envelopes[0].id, 1);
    EXPECT_EQ(parsed.cluster_view.envelopes[0].payload_hash, "hash1");
    EXPECT_EQ(parsed.cluster_view.envelopes[1].id, 2);
    EXPECT_EQ(parsed.cluster_view.envelopes[1].payload_hash, "hash2");
}

TEST(ExternalBindingsTest, FormatDoubleStable) {
    ExternalClusterView view;
    view.node_count = 1;

    view.coherence = 99.5;
    std::string json = JsonBindings::to_json(view);
    EXPECT_NE(json.find("\"coherence\":99.50"), std::string::npos);

    view.coherence = 33.333333; // should round to .33
    json = JsonBindings::to_json(view);
    EXPECT_NE(json.find("\"coherence\":33.33"), std::string::npos);

    view.coherence = 0.0;
    json = JsonBindings::to_json(view);
    EXPECT_NE(json.find("\"coherence\":0.00"), std::string::npos);
}

TEST(ExternalBindingsTest, FederationExportCorrectness) {
    ClusterView internal_view;
    internal_view.total_nodes = 3;
    internal_view.coherence_summary.global_coherence_score = 88;

    MeshPropagationEnvelope env1{};
    env1.source_node_id_hash = 20;
    env1.anchor.epoch.epoch_height = 100;

    MeshPropagationEnvelope env2{};
    env2.source_node_id_hash = 10;
    env2.anchor.epoch.epoch_height = 200;

    internal_view.mesh_envelopes.push_back(env1);
    internal_view.mesh_envelopes.push_back(env2);

    FederationExport fed_export;
    ExternalClusterView ext_view = fed_export.export_view(internal_view);

    EXPECT_EQ(ext_view.node_count, 3);
    EXPECT_TRUE(std::abs(ext_view.coherence - 88.0) < 0.001);

    // Check sorting (env2 with ID 10 should come before env1 with ID 20)
    ASSERT_EQ(ext_view.envelopes.size(), 2);
    EXPECT_EQ(ext_view.envelopes[0].id, 10);
    EXPECT_EQ(ext_view.envelopes[0].timestamp, 200);
    EXPECT_EQ(ext_view.envelopes[1].id, 20);
    EXPECT_EQ(ext_view.envelopes[1].timestamp, 100);
}

TEST(ExternalBindingsTest, ReplayExportCorrectness) {
    ReplayTick internal_tick;
    internal_tick.telemetry.total_nodes = 4;
    internal_tick.telemetry.in_sync_nodes = 4;
    internal_tick.telemetry.global_coherence_score = 100;
    internal_tick.telemetry.epoch_height = 10;
    internal_tick.telemetry.tick_count = 50;

    internal_tick.view.total_nodes = 4;
    internal_tick.view.coherence_summary.global_coherence_score = 100;

    ReplayExport replay_export;
    ExternalReplayTick ext_tick = replay_export.export_tick(5, internal_tick);

    EXPECT_EQ(ext_tick.tick_index, 5);
    EXPECT_EQ(ext_tick.cluster_view.node_count, 4);
    EXPECT_TRUE(std::abs(ext_tick.cluster_view.coherence - 100.0) < 0.001);

    // Telemetry json must contain specific fields
    EXPECT_NE(ext_tick.telemetry_json.find("\"total_nodes\":4"), std::string::npos);
    EXPECT_NE(ext_tick.telemetry_json.find("\"in_sync_nodes\":4"), std::string::npos);
    EXPECT_NE(ext_tick.telemetry_json.find("\"global_coherence_score\":100"), std::string::npos);

    EXPECT_FALSE(ext_tick.scheduler_state_hash.empty());
}
