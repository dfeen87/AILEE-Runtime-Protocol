#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"
#include "l6/ZKMockBackend.h"

using namespace ailee::l6;

TEST(ZKProvingIntegrationTest, OrchestrationAttachesValidProof) {
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::MOCK, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    OrchestrationContext ctx;
    ctx.epoch_id = 42;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    std::string state_root_hash = "deadbeef";

    auto result = orchestrate_epoch(ctx, &backend, config, &constraints, &transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.epoch_id, 42);
    EXPECT_EQ(payload.state_root_hash, "deadbeef");
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "constraint_1");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "transcript_1");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);

    // Proof commitment hash should not be empty
    EXPECT_FALSE(payload.proof_commitment_hash.empty());
}

TEST(ZKProvingIntegrationTest, OrchestrationLeavesEmptyWhenSkipped) {
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::MOCK, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    OrchestrationContext ctx;
    ctx.epoch_id = 43;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::SKIP;

    std::string state_root_hash = "deadbeef";

    auto result = orchestrate_epoch(ctx, &backend, config, &constraints, &transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_FALSE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}
