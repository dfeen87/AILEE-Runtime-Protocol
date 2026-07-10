#include <gtest/gtest.h>
#include "l6/OnChainReplayVerifier.h"

namespace ailee {
namespace l6 {

TEST(OnChainReplayVerifierTest, MissingAnchorHash) {
    ReplayVerdict verdict = verify_epoch_against_anchor(1, "abc", std::nullopt);
    EXPECT_EQ(verdict.status, ReplayStatus::MISSING_ANCHOR);
    EXPECT_EQ(verdict.epoch_id, 1);
    EXPECT_EQ(verdict.local_hash, "abc");
    EXPECT_EQ(verdict.anchor_hash, "");
}

TEST(OnChainReplayVerifierTest, MismatchAnchorHash) {
    ReplayVerdict verdict = verify_epoch_against_anchor(2, "abc", "def");
    EXPECT_EQ(verdict.status, ReplayStatus::MISMATCH);
    EXPECT_EQ(verdict.epoch_id, 2);
    EXPECT_EQ(verdict.local_hash, "abc");
    EXPECT_EQ(verdict.anchor_hash, "def");
}

TEST(OnChainReplayVerifierTest, MatchAnchorHash) {
    ReplayVerdict verdict = verify_epoch_against_anchor(3, "abc", "abc");
    EXPECT_EQ(verdict.status, ReplayStatus::OK);
    EXPECT_EQ(verdict.epoch_id, 3);
    EXPECT_EQ(verdict.local_hash, "abc");
    EXPECT_EQ(verdict.anchor_hash, "abc");
}

} // namespace l6
} // namespace ailee
