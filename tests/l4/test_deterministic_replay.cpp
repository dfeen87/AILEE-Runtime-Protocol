#include <gtest/gtest.h>
#include "l4/ClusterSim.h"
#include "l4/ReplayBuffer.h"
#include "l4/ReplayEngine.h"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>

using namespace ailee::l4;

class DeterministicReplayTest : public ::testing::Test {
public:
    void SetUp() override {
        std::remove("web/replay.bin");
        std::remove("web/replay2.bin");
    }

    void TearDown() override {
        std::remove("web/replay.bin");
        std::remove("web/replay2.bin");
    }
};

TEST_F(DeterministicReplayTest, ReplayMatchesOriginalRun) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (int i = 0; i < 3; ++i) {
        initial_nodes[i].engine_state.epoch.epoch_number = 0;
    }
    std::vector<std::pair<size_t, size_t>> gossip_schedule = {{0, 1}, {1, 2}, {2, 0}};
    
    // Max steps = 5 full ticks
    ClusterView view = run_cluster_simulation(initial_nodes, gossip_schedule, 5);

    ReplayEngine engine;
    bool load_success = true;
    try {
        engine.load_replay_file("web/replay.bin");
    } catch (...) {
        load_success = false;
    }
    EXPECT_TRUE(load_success);

    // We simulated 5 full ticks. The buffer should have 5 snapshots.
    EXPECT_EQ(engine.buffer.scheduler_snapshots.size(), 5);

    // Ensure final state exactly matches the final loaded state in tick index 4
    DeterministicSchedulerState scheduler;
    ClusterView v;
    TelemetrySample t;
    
    EXPECT_TRUE(engine.replay_tick(4, scheduler, v, t));
    
    // Assert the loaded tick value aligns
    EXPECT_EQ(scheduler.tick_count, 45); 
    
    // Pass in the original returned `view` output from the live simulation and verify it against the loaded buffer state
    EXPECT_TRUE(engine.verify_tick(4, scheduler, view, t));
}

TEST_F(DeterministicReplayTest, DeterministicAcrossRuns) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (int i = 0; i < 3; ++i) {
        initial_nodes[i].engine_state.epoch.epoch_number = 0;
    }
    std::vector<std::pair<size_t, size_t>> gossip_schedule = {{0, 1}, {1, 2}, {2, 0}};
    
    // First run
    run_cluster_simulation(initial_nodes, gossip_schedule, 3);
    std::rename("web/replay.bin", "web/replay1.bin");

    // Second run
    run_cluster_simulation(initial_nodes, gossip_schedule, 3);
    std::rename("web/replay.bin", "web/replay2.bin");

    // Compare binary files exactly
    std::ifstream file1("web/replay1.bin", std::ios::binary | std::ios::ate);
    std::ifstream file2("web/replay2.bin", std::ios::binary | std::ios::ate);
    
    ASSERT_TRUE(file1.is_open());
    ASSERT_TRUE(file2.is_open());
    
    std::streamsize size1 = file1.tellg();
    std::streamsize size2 = file2.tellg();
    
    EXPECT_EQ(size1, size2);
    
    file1.seekg(0, std::ios::beg);
    file2.seekg(0, std::ios::beg);
    
    std::vector<char> buf1(size1);
    std::vector<char> buf2(size2);
    
    if (file1.read(buf1.data(), size1) && file2.read(buf2.data(), size2)) {
        EXPECT_EQ(std::memcmp(buf1.data(), buf2.data(), size1), 0);
    } else {
        EXPECT_TRUE(false);
    }

    std::remove("web/replay1.bin");
}
