#include <gtest/gtest.h>
#include "l6/isla/IslaModeEngine.hpp"

using namespace ailee::l6::isla;

class IslaModeEngineTest : public ::testing::Test {
protected:
    IslaModeEngine engine;

    IslaTuningDecision base_decision = {
        100,  // batch size
        1000, // proof interval
        8,    // workers
        AnchorCadence::NORMAL
    };
};

TEST_F(IslaModeEngineTest, CoherenceScoreCalculation) {
    EpochMetricsWindow window;
    window.epochs.push_back({0.0, 0, 100.0, 100.0}); // Perfect
    window.epochs.push_back({0.0, 0, 100.0, 100.0}); // Perfect

    double score = IslaModeEngine::computeCoherenceScore(window);
    EXPECT_DOUBLE_EQ(score, 1.0);

    window.epochs.push_back({0.5, 0, 100.0, 100.0}); // 1.0 - 0.25 = 0.75
    score = IslaModeEngine::computeCoherenceScore(window);
    EXPECT_DOUBLE_EQ(score, (1.0 + 1.0 + 0.75) / 3.0);

    window.epochs.push_back({0.0, 1, 100.0, 100.0}); // 1.0 - 0.2 = 0.8
    score = IslaModeEngine::computeCoherenceScore(window);
    EXPECT_DOUBLE_EQ(score, (1.0 + 1.0 + 0.75 + 0.8) / 4.0);
}

TEST_F(IslaModeEngineTest, EmptyWindowsReturnNeutral) {
    EpochMetricsWindow ep_win;
    PerformanceMetricsWindow pf_win;
    EconomicMetricsWindow ec_win;

    auto decision = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    EXPECT_EQ(decision.new_batch_size, 100);
    EXPECT_EQ(decision.new_proof_interval_ms, 1000);
    EXPECT_EQ(decision.new_worker_allocation, 8);
    EXPECT_EQ(decision.new_anchor_cadence, AnchorCadence::NORMAL);
}

TEST_F(IslaModeEngineTest, IncreaseBatchSizeOnHighCoherence) {
    EpochMetricsWindow ep_win;
    ep_win.epochs.push_back({0.0, 0, 10.0, 10.0}); // coherence = 1.0

    PerformanceMetricsWindow pf_win;
    pf_win.epochs.push_back({100.0, 0.5, 1.0}); // performance = 1.0

    EconomicMetricsWindow ec_win;

    auto decision = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    // Batch size should increase by 10% (100 * 1.1 = 110)
    EXPECT_EQ(decision.new_batch_size, 110);
}

TEST_F(IslaModeEngineTest, DecreaseBatchSizeOnLowCoherence) {
    EpochMetricsWindow ep_win;
    ep_win.epochs.push_back({0.0, 3, 2000.0, 10.0}); // bad coherence (score = 1 - 0.6 - 0.1 = 0.3)

    PerformanceMetricsWindow pf_win;
    EconomicMetricsWindow ec_win;

    auto decision = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    // Batch size should decrease by 20% (100 -> 80)
    EXPECT_EQ(decision.new_batch_size, 80);
    EXPECT_EQ(decision.new_anchor_cadence, AnchorCadence::TIGHT); // also sets TIGHT cadence
}

TEST_F(IslaModeEngineTest, ClampBatchSizeToMinOnHighDispute) {
    EpochMetricsWindow ep_win;
    ep_win.epochs.push_back({0.5, 0, 10.0, 10.0}); // High dispute rate

    PerformanceMetricsWindow pf_win;
    EconomicMetricsWindow ec_win;

    auto decision = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    // avg_dispute_rate > 0.1 -> clamp to min safe
    EXPECT_EQ(decision.new_batch_size, 1);
    EXPECT_EQ(decision.new_anchor_cadence, AnchorCadence::TIGHT);
}

TEST_F(IslaModeEngineTest, AdjustProofInterval) {
    EpochMetricsWindow ep_win;
    ep_win.epochs.push_back({0.0, 0, 10.0, 10.0}); // anchor_time < 500

    PerformanceMetricsWindow pf_win;

    EconomicMetricsWindow ec_win;
    ec_win.epochs.push_back({5.0, 0.1, 0.1}); // fee < 10

    auto decision = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    // Shorten interval by 10%
    EXPECT_EQ(decision.new_proof_interval_ms, 900);

    // Now test lengthen
    ep_win.epochs[0].anchor_confirmation_time = 3000.0;
    decision = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    // Lengthen interval by 15%
    EXPECT_EQ(decision.new_proof_interval_ms, 1150);
}

TEST_F(IslaModeEngineTest, WorkerAllocationLimits) {
    EpochMetricsWindow ep_win;
    PerformanceMetricsWindow pf_win;
    pf_win.epochs.push_back({3000.0, 0.1, 1.0}); // high time, low util -> increase
    EconomicMetricsWindow ec_win;

    IslaTuningDecision d = base_decision;
    d.new_worker_allocation = 64; // Max limit

    auto decision = engine.computeDecision(ep_win, pf_win, ec_win, d);
    // Should clamp to 64
    EXPECT_EQ(decision.new_worker_allocation, 64);
}

TEST_F(IslaModeEngineTest, NoRandomnessDeterminismTest) {
    EpochMetricsWindow ep_win;
    ep_win.epochs.push_back({0.1, 1, 500.0, 1500.0});

    PerformanceMetricsWindow pf_win;
    pf_win.epochs.push_back({2000.0, 0.8, 0.9});

    EconomicMetricsWindow ec_win;
    ec_win.epochs.push_back({25.0, 0.3, 0.5});

    auto decision1 = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    // Run exactly the same inputs again
    auto decision2 = engine.computeDecision(ep_win, pf_win, ec_win, base_decision);

    EXPECT_EQ(decision1.new_batch_size, decision2.new_batch_size);
    EXPECT_EQ(decision1.new_proof_interval_ms, decision2.new_proof_interval_ms);
    EXPECT_EQ(decision1.new_worker_allocation, decision2.new_worker_allocation);
    EXPECT_EQ(decision1.new_anchor_cadence, decision2.new_anchor_cadence);
}
