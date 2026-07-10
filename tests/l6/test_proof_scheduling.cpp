#include <gtest/gtest.h>
#include "l6/ProofScheduling.h"

namespace ailee::l6::tests {

TEST(ProofSchedulingTest, ZeroIntervalEpochsAlwaysSkips) {
    ProofSchedulingConfig config;
    config.proof_interval_epochs = 0;
    config.max_proof_window_size = 10;

    ProofPlan plan = compute_proof_plan(5, config);

    EXPECT_EQ(plan.decision, ProofDecision::SKIP);
    EXPECT_EQ(plan.epoch_id, 5);
    EXPECT_EQ(plan.proof_cycle_id, 0);
}

TEST(ProofSchedulingTest, ExactIntervalEpochAttachesProof) {
    ProofSchedulingConfig config;
    config.proof_interval_epochs = 10;
    config.max_proof_window_size = 10;

    ProofPlan plan = compute_proof_plan(30, config);

    EXPECT_EQ(plan.decision, ProofDecision::ATTACH_PROOF);
    EXPECT_EQ(plan.epoch_id, 30);
    EXPECT_EQ(plan.proof_cycle_id, 3);
}

TEST(ProofSchedulingTest, OffIntervalEpochSkipsWithDeterministicCycleId) {
    ProofSchedulingConfig config;
    config.proof_interval_epochs = 10;
    config.max_proof_window_size = 10;

    ProofPlan plan = compute_proof_plan(35, config);

    EXPECT_EQ(plan.decision, ProofDecision::SKIP);
    EXPECT_EQ(plan.epoch_id, 35);
    EXPECT_EQ(plan.proof_cycle_id, 3);
}

} // namespace ailee::l6::tests
