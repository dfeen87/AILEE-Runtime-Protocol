#include "l6/AnchorVerifier.h"
#include <gtest/gtest.h>

using namespace ailee::l6;

TEST(AnchorVerifierTest, ExactMatchReturnsTrue) {
    AnchorRecord rec;
    rec.version = 1;
    rec.replay_height = 100;
    rec.coherence_score = 50;
    rec.state_root.fill(0xAA);

    AnchorState state;
    state.replay_height = 100;
    state.coherence_score = 50;
    state.state_root.fill(0xAA);

    AnchorVerifier verifier;
    EXPECT_TRUE(verifier.verify(rec, state));
}

TEST(AnchorVerifierTest, MismatchedReplayHeightReturnsFalse) {
    AnchorRecord rec;
    rec.version = 1;
    rec.replay_height = 100;
    rec.coherence_score = 50;
    rec.state_root.fill(0xAA);

    AnchorState state;
    state.replay_height = 101;
    state.coherence_score = 50;
    state.state_root.fill(0xAA);

    AnchorVerifier verifier;
    EXPECT_FALSE(verifier.verify(rec, state));
}

TEST(AnchorVerifierTest, MismatchedCoherenceScoreReturnsFalse) {
    AnchorRecord rec;
    rec.version = 1;
    rec.replay_height = 100;
    rec.coherence_score = 50;
    rec.state_root.fill(0xAA);

    AnchorState state;
    state.replay_height = 100;
    state.coherence_score = 51;
    state.state_root.fill(0xAA);

    AnchorVerifier verifier;
    EXPECT_FALSE(verifier.verify(rec, state));
}

TEST(AnchorVerifierTest, MismatchedStateRootReturnsFalse) {
    AnchorRecord rec;
    rec.version = 1;
    rec.replay_height = 100;
    rec.coherence_score = 50;
    rec.state_root.fill(0xAA);

    AnchorState state;
    state.replay_height = 100;
    state.coherence_score = 50;
    state.state_root.fill(0xAB); // Different root

    AnchorVerifier verifier;
    EXPECT_FALSE(verifier.verify(rec, state));
}
