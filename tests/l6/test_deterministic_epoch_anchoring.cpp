#include <gtest/gtest.h>
#include "l6/DeterministicEpochAnchoring.h"

using namespace ailee::l6;

TEST(DeterministicEpochAnchoringTest, IntervalZero) {
    AnchorPlan plan = compute_anchor_plan(10, 0);
    EXPECT_EQ(plan.decision, AnchorDecision::SKIP);
    EXPECT_EQ(plan.epoch_id, 10);
    EXPECT_EQ(plan.anchor_cycle_id, 0);
}

TEST(DeterministicEpochAnchoringTest, EpochZero) {
    AnchorPlan plan = compute_anchor_plan(0, 10);
    EXPECT_EQ(plan.decision, AnchorDecision::SKIP);
    EXPECT_EQ(plan.epoch_id, 0);
    EXPECT_EQ(plan.anchor_cycle_id, 0);
}

TEST(DeterministicEpochAnchoringTest, EpochMatchesInterval) {
    AnchorPlan plan = compute_anchor_plan(10, 10);
    EXPECT_EQ(plan.decision, AnchorDecision::ANCHOR);
    EXPECT_EQ(plan.epoch_id, 10);
    EXPECT_EQ(plan.anchor_cycle_id, 1);
}

TEST(DeterministicEpochAnchoringTest, EpochTwiceInterval) {
    AnchorPlan plan = compute_anchor_plan(20, 10);
    EXPECT_EQ(plan.decision, AnchorDecision::ANCHOR);
    EXPECT_EQ(plan.epoch_id, 20);
    EXPECT_EQ(plan.anchor_cycle_id, 2);
}

TEST(DeterministicEpochAnchoringTest, EpochSkips) {
    AnchorPlan plan = compute_anchor_plan(21, 10);
    EXPECT_EQ(plan.decision, AnchorDecision::SKIP);
    EXPECT_EQ(plan.epoch_id, 21);
    EXPECT_EQ(plan.anchor_cycle_id, 2);

    AnchorPlan plan2 = compute_anchor_plan(25, 10);
    EXPECT_EQ(plan2.decision, AnchorDecision::SKIP);
    EXPECT_EQ(plan2.epoch_id, 25);
    EXPECT_EQ(plan2.anchor_cycle_id, 2);

    AnchorPlan plan3 = compute_anchor_plan(9, 10);
    EXPECT_EQ(plan3.decision, AnchorDecision::SKIP);
    EXPECT_EQ(plan3.epoch_id, 9);
    EXPECT_EQ(plan3.anchor_cycle_id, 0);
}

TEST(DeterministicEpochAnchoringTest, LargeValues) {
    uint64_t large_epoch = 1000000000000ULL;
    uint64_t large_interval = 3000ULL;
    AnchorPlan plan = compute_anchor_plan(large_epoch, large_interval);
    EXPECT_EQ(plan.decision, AnchorDecision::SKIP);
    EXPECT_EQ(plan.epoch_id, large_epoch);
    EXPECT_EQ(plan.anchor_cycle_id, large_epoch / large_interval);

    uint64_t exact_epoch = 3000000000000ULL;
    AnchorPlan exact_plan = compute_anchor_plan(exact_epoch, large_interval);
    EXPECT_EQ(exact_plan.decision, AnchorDecision::ANCHOR);
    EXPECT_EQ(exact_plan.epoch_id, exact_epoch);
    EXPECT_EQ(exact_plan.anchor_cycle_id, exact_epoch / large_interval);
}
