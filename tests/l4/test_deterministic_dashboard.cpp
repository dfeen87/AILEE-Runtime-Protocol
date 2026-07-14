#include <gtest/gtest.h>
#include "l4/DeterministicDashboard.h"
#include "l4/ClusterSim.h"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

using namespace ailee::l4;

class DeterministicDashboardTest : public ::testing::Test {
public:
    void SetUp() override {
        // Setup initial dummy telemetry buffer
        sample1 = {};
        sample1.tick_count = 1;
        sample1.epoch_height = 10;
        sample1.total_nodes = 5;
        sample1.in_sync_nodes = 4;
        sample1.consistent_state_root_nodes = 4;
        sample1.inconsistent_state_root_nodes = 0;
        sample1.global_coherence_score = 80;

        sample2 = {};
        sample2.tick_count = 2;
        sample2.epoch_height = 10;
        sample2.total_nodes = 5;
        sample2.in_sync_nodes = 5;
        sample2.consistent_state_root_nodes = 5;
        sample2.inconsistent_state_root_nodes = 0;
        sample2.global_coherence_score = 100;

        buffer.samples.push_back(sample1);
        buffer.samples.push_back(sample2);
    }

    TelemetrySample sample1;
    TelemetrySample sample2;
    TelemetryBuffer buffer;
    DashboardBuilder builder;
};

TEST_F(DeterministicDashboardTest, DeterministicJsonGeneration) {
    DashboardSnapshot snap1 = builder.build_snapshot(buffer);
    DashboardSnapshot snap2 = builder.build_snapshot(buffer);

    // Build snapshot twice -> strings must match exactly
    ASSERT_EQ(snap1.json, snap2.json);
}

TEST_F(DeterministicDashboardTest, JsonCorrectness) {
    DashboardSnapshot snap = builder.build_snapshot(buffer);

    // Verify JSON matches telemetry
    std::string expected_json = "{ \"telemetry\": [{"
        "\"tick\":1,\"epoch\":10,\"total_nodes\":5,\"in_sync_nodes\":4,"
        "\"consistent_state_root_nodes\":4,\"inconsistent_state_root_nodes\":0,\"global_coherence_score\":80},"
        "{\"tick\":2,\"epoch\":10,\"total_nodes\":5,\"in_sync_nodes\":5,"
        "\"consistent_state_root_nodes\":5,\"inconsistent_state_root_nodes\":0,\"global_coherence_score\":100}"
        "] }";

    ASSERT_EQ(snap.json, expected_json);
}

TEST_F(DeterministicDashboardTest, IntegrationTest) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (size_t i = 0; i < 3; i++) {
        initial_nodes[i] = ClusterNodeState{};
        initial_nodes[i].node_id_hash = i + 1;
    }

    std::vector<std::pair<size_t, size_t>> schedule = { {0, 1}, {1, 2}, {2, 0} };

    // Ensure web directory exists for tests, but we'll try reading what it actually wrote
    // The test environment might not have a web directory where it's being run.
    std::filesystem::create_directories("web");

    // Run N ticks
    run_cluster_simulation(initial_nodes, schedule, 2);

    // Write dashboard.json should have happened inside run_cluster_simulation.
    // Read and verify
    std::ifstream ifs("web/dashboard.json");
    ASSERT_TRUE(ifs.is_open());

    std::stringstream json_stream;
    json_stream << ifs.rdbuf();
    std::string json_content = json_stream.str();

    // Verify JSON content is somewhat correct
    ASSERT_TRUE(json_content.find("\"telemetry\": [") != std::string::npos);
    ASSERT_TRUE(json_content.find("\"tick\":") != std::string::npos);
}

TEST_F(DeterministicDashboardTest, NoNondeterministicFields) {
    DashboardSnapshot snap = builder.build_snapshot(buffer);

    // Assert JSON contains no float (.) or timestamps
    // We expect our simple expected_json not to have random data
    ASSERT_EQ(snap.json.find('.'), std::string::npos);
}
