#include <gtest/gtest.h>
#include "policy/PolicyEngine.h"

using namespace ailee::policy;
using namespace ailee::l6;

TEST(PolicyEngineTest, HighCoherenceAndExactFrequencyAnchors) {
    PolicyConfig config{80, 10, true, "PROD"};
    PolicyEngine engine(config);

    SchedulerDecision decision = engine.compute_epoch_decision(20, 0.95);
    EXPECT_EQ(decision.anchor_decision, AnchorDecision::ANCHOR);
    EXPECT_EQ(decision.proof_decision, ProofDecision::ATTACH_PROOF);
}

TEST(PolicyEngineTest, LowCoherenceSkipsAnchorAndProofStrict) {
    PolicyConfig config{80, 10, true, "PROD"};
    PolicyEngine engine(config);

    SchedulerDecision decision = engine.compute_epoch_decision(20, 0.50);
    EXPECT_EQ(decision.anchor_decision, AnchorDecision::SKIP);
    EXPECT_EQ(decision.proof_decision, ProofDecision::SKIP);
}

TEST(PolicyEngineTest, BackendAllowedInCI) {
    PolicyConfig config{80, 10, true, "PROD"};
    PolicyEngine engine(config);

    EXPECT_TRUE(engine.is_backend_allowed(ZKBackendType::MOCK, true));
    EXPECT_FALSE(engine.is_backend_allowed(ZKBackendType::HALO2_NATIVE, true));
}

TEST(PolicyEngineTest, BackendAllowedInProd) {
    PolicyConfig config{80, 10, true, "PROD"};
    PolicyEngine engine(config);

    EXPECT_FALSE(engine.is_backend_allowed(ZKBackendType::MOCK, false));
    EXPECT_TRUE(engine.is_backend_allowed(ZKBackendType::HALO2_NATIVE, false));
}
