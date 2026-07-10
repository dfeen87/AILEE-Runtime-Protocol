#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"

namespace ailee::l6::tests {

TEST(ZKOrchestrationManagerTest, NoAnchorNoProof) {
    OrchestrationContext ctx;
    ctx.epoch_id = 100;
    ctx.anchor_plan.decision = AnchorDecision::SKIP;
    ctx.proof_plan.decision = ProofDecision::SKIP;

    OrchestrationResult result = orchestrate_epoch(ctx, nullptr, "state_root_100");

    EXPECT_FALSE(result.should_anchor);
    EXPECT_FALSE(result.should_attach_proof);

    EXPECT_EQ(result.anchor_payload.epoch_id, 100);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_100");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "");
}

TEST(ZKOrchestrationManagerTest, AnchorNoProof) {
    OrchestrationContext ctx;
    ctx.epoch_id = 101;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::SKIP;

    OrchestrationResult result = orchestrate_epoch(ctx, nullptr, "state_root_101");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_FALSE(result.should_attach_proof);

    EXPECT_EQ(result.anchor_payload.epoch_id, 101);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_101");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "");
}

TEST(ZKOrchestrationManagerTest, AnchorAndProofWithValidStub) {
    OrchestrationContext ctx;
    ctx.epoch_id = 102;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    ZKProofStub stub;
    stub.proof_id = "proof_abc";
    stub.constraint_set_id = "cs_xyz";
    stub.transcript_id = "tr_123";
    stub.size_bytes = 4096;

    OrchestrationResult result = orchestrate_epoch(ctx, &stub, "state_root_102");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    EXPECT_EQ(result.anchor_payload.epoch_id, 102);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_102");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "cs_xyz");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "tr_123");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "proof_abc");
}

TEST(ZKOrchestrationManagerTest, AttachProofButNullStub) {
    OrchestrationContext ctx;
    ctx.epoch_id = 103;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    // proof_stub is explicitly nullptr, meaning no proof is actually available
    OrchestrationResult result = orchestrate_epoch(ctx, nullptr, "state_root_103");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_FALSE(result.should_attach_proof); // Must be false since stub is null

    EXPECT_EQ(result.anchor_payload.epoch_id, 103);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_103");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "");
}

} // namespace ailee::l6::tests
