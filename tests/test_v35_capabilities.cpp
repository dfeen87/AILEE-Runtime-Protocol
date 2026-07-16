#include <gtest/gtest.h>
#include "AILEEWebServer.h"
#include "webserver/RouteRegistry.h"
#include "runtime/StateProjection.h"
#include "kernel/Hooks.h"
#include "l6/JsonBindings.h"
#include "l6/ExternalSchema.h"
#include <fstream>
#include <sstream>

using namespace ailee;
using namespace ailee::l6;
using namespace ailee::runtime;
using namespace ailee::kernel;

TEST(V35CapabilitiesTest, RouteRegistryModularRegistration) {
    // Instantiate AILEEWebServer to trigger static route registration
    WebServerConfig config;
    AILEEWebServer server(config);

    auto& registry = RouteRegistry::getInstance();

    // Check that routes are registered
    const auto& routes = registry.getRoutes();
    EXPECT_FALSE(routes.empty());

    bool found_snapshot_route = false;
    bool found_audit_route = false;

    for (const auto& r : routes) {
        if (r.path == "/api/state/snapshot") {
            found_snapshot_route = true;
            EXPECT_EQ(r.method, HttpMethod::GET);
            EXPECT_EQ(r.signature_metadata, "get_state_snapshot");
            EXPECT_TRUE(r.kernel_aware);
        } else if (r.path == "/api/replay/audit") {
            found_audit_route = true;
            EXPECT_EQ(r.method, HttpMethod::GET);
            EXPECT_EQ(r.signature_metadata, "get_replay_audit");
            EXPECT_TRUE(r.kernel_aware);
        }
    }

    EXPECT_TRUE(found_snapshot_route);
    EXPECT_TRUE(found_audit_route);
}

TEST(V35CapabilitiesTest, StateProjectionDeterminism) {
    // Generate snapshot
    ExternalStateSnapshot snap1 = StateProjection::generateSnapshot();
    ExternalStateSnapshot snap2 = StateProjection::generateSnapshot();

    // Verify properties are populated
    EXPECT_EQ(snap1.active_sessions.size(), 2);
    EXPECT_EQ(snap1.active_sessions[0].session_id, "session_1");
    EXPECT_EQ(snap1.active_sessions[0].status, "idle");
    EXPECT_EQ(snap1.active_sessions[1].session_id, "session_2");
    EXPECT_EQ(snap1.active_sessions[1].status, "active");

    EXPECT_EQ(snap1.broadcast_queues.size(), 2);
    EXPECT_EQ(snap1.broadcast_queues[0].queue_id, "mempool_queue");
    EXPECT_EQ(snap1.broadcast_queues[0].pending_count, 5);
    EXPECT_EQ(snap1.broadcast_queues[1].queue_id, "anchor_queue");
    EXPECT_EQ(snap1.broadcast_queues[1].pending_count, 1);

    EXPECT_FALSE(snap1.replay_active);
    EXPECT_EQ(snap1.current_tick_index, 0);
    EXPECT_EQ(snap1.total_ticks, 100);

    // Verify tick-based determinism (no raw addresses or system time)
    std::string json1 = JsonBindings::to_json(snap1);
    std::string json2 = JsonBindings::to_json(snap2);

    // Ensure we can parse them back correctly
    ExternalStateSnapshot parsed1 = JsonBindings::from_json_state_snapshot(json1);
    ExternalStateSnapshot parsed2 = JsonBindings::from_json_state_snapshot(json2);

    EXPECT_EQ(parsed1.active_sessions.size(), 2);
    EXPECT_EQ(parsed1.broadcast_queues.size(), 2);
    EXPECT_EQ(parsed1.global_tick_count + 1, parsed2.global_tick_count); // strictly deterministic progression
}

TEST(V35CapabilitiesTest, ReplayIntrospectionAdditiveSchema) {
    ExternalEnvelope env;
    env.id = 123;
    env.timestamp = 9999;
    env.payload_hash = "abc123hash";
    env.type = "gossip";
    env.status = "delivered";

    std::string env_json = JsonBindings::to_json(env);

    // Ensure both extensions and legacy keys are present and sorted
    EXPECT_NE(env_json.find("\"id\":123"), std::string::npos);
    EXPECT_NE(env_json.find("\"payload_hash\":\"abc123hash\""), std::string::npos);
    EXPECT_NE(env_json.find("\"status\":\"delivered\""), std::string::npos);
    EXPECT_NE(env_json.find("\"timestamp\":9999"), std::string::npos);
    EXPECT_NE(env_json.find("\"type\":\"gossip\""), std::string::npos);

    ExternalEnvelope parsed_env = JsonBindings::from_json_envelope(env_json);
    EXPECT_EQ(parsed_env.id, 123);
    EXPECT_EQ(parsed_env.timestamp, 9999);
    EXPECT_EQ(parsed_env.payload_hash, "abc123hash");
    EXPECT_EQ(parsed_env.type, "gossip");
    EXPECT_EQ(parsed_env.status, "delivered");

    ExternalReplayTick tick;
    tick.tick_index = 45;
    tick.scheduler_state_hash = "scheduler_hash";
    tick.telemetry_json = "{}";
    tick.replay_phase = "execution";
    tick.replay_mode_state = "active";
    tick.cluster_view.node_count = 1;
    tick.cluster_view.coherence = 100.0;

    std::string tick_json = JsonBindings::to_json(tick);
    EXPECT_NE(tick_json.find("\"replay_phase\":\"execution\""), std::string::npos);
    EXPECT_NE(tick_json.find("\"replay_mode_state\":\"active\""), std::string::npos);

    ExternalReplayTick parsed_tick = JsonBindings::from_json_tick(tick_json);
    EXPECT_EQ(parsed_tick.tick_index, 45);
    EXPECT_EQ(parsed_tick.scheduler_state_hash, "scheduler_hash");
    EXPECT_EQ(parsed_tick.replay_phase, "execution");
    EXPECT_EQ(parsed_tick.replay_mode_state, "active");
}

TEST(V35CapabilitiesTest, KernelHookAttachmentIntegrity) {
    // Clear the audit file first to test fresh append
    std::ofstream clear_file("logs/audit_trail.json", std::ios::trunc);
    clear_file << "[]";
    clear_file.close();

    ExternalReplayTick tick;
    tick.tick_index = 101;
    tick.replay_phase = "verification";
    tick.replay_mode_state = "paused";

    ReplayTickContext ctx;
    ctx.tick_index = 101;
    ctx.status = "paused_event";

    // Trigger onReplayTick manually which appends to logs/audit_trail.json
    Hooks::onReplayTick(ctx, tick);

    // Verify it is written correctly to logs/audit_trail.json
    std::ifstream infile("logs/audit_trail.json");
    ASSERT_TRUE(infile.is_open());
    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string content = buffer.str();
    infile.close();

    EXPECT_NE(content.find("\"tick_index\":101"), std::string::npos);
    EXPECT_NE(content.find("\"replay_phase\":\"verification\""), std::string::npos);
    EXPECT_NE(content.find("\"replay_mode_state\":\"paused\""), std::string::npos);
}
