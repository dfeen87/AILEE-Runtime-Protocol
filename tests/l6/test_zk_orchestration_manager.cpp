#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"

namespace ailee::l6::tests {

TEST(ZKOrchestrationManagerTest, NoAnchorNoProof) {
    OrchestrationContext ctx;
    ctx.epoch_id = 100;
    ctx.anchor_plan.decision = AnchorDecision::SKIP;
    ctx.proof_plan.decision = ProofDecision::SKIP;

    OrchestrationResult result = orchestrate_epoch(ctx, nullptr, nullptr, nullptr, "state_root_100");

    EXPECT_FALSE(result.should_anchor);
    EXPECT_FALSE(result.should_attach_proof);

    EXPECT_EQ(result.anchor_payload.epoch_id, 100);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_100");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
}

TEST(ZKOrchestrationManagerTest, AnchorNoProof) {
    OrchestrationContext ctx;
    ctx.epoch_id = 101;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::SKIP;

    OrchestrationResult result = orchestrate_epoch(ctx, nullptr, nullptr, nullptr, "state_root_101");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_FALSE(result.should_attach_proof);

    EXPECT_EQ(result.anchor_payload.epoch_id, 101);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_101");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
}

TEST(ZKOrchestrationManagerTest, AnchorAndProofWithValidMetadata) {
    OrchestrationContext ctx;
    ctx.epoch_id = 102;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    ZKProofMetadata metadata;
    metadata.proof_id = "proof_abc";
    metadata.constraint_set_id = "cs_xyz";
    metadata.transcript_id = "tr_123";
    metadata.logical_size_bytes = 4096;

    ZKConstraintSet constraints{"cs_xyz", 10};
    ZKTranscript transcript{"tr_123", 5};

    OrchestrationResult result = orchestrate_epoch(ctx, &metadata, &constraints, &transcript, "state_root_102");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    EXPECT_EQ(result.anchor_payload.epoch_id, 102);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_102");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "cs_xyz");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "tr_123");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "proof_abc");
    EXPECT_EQ(result.anchor_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
}

TEST(ZKOrchestrationManagerTest, AttachProofButNullMetadata) {
    OrchestrationContext ctx;
    ctx.epoch_id = 103;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    // proof_metadata is explicitly nullptr, meaning no proof is actually available
    OrchestrationResult result = orchestrate_epoch(ctx, nullptr, nullptr, nullptr, "state_root_103");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof); // scheduling says attach

    EXPECT_EQ(result.anchor_payload.epoch_id, 103);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_103");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.validation_status, DeterministicZKStatus::OK); // Default when metadata is null
}

TEST(ZKOrchestrationManagerTest, AttachProofWithInvalidMetadata) {
    OrchestrationContext ctx;
    ctx.epoch_id = 104;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    ZKProofMetadata metadata;
    metadata.proof_id = "proof_abc";
    metadata.constraint_set_id = "cs_wrong";
    metadata.transcript_id = "tr_123";
    metadata.logical_size_bytes = 4096;

    ZKConstraintSet constraints{"cs_xyz", 10};
    ZKTranscript transcript{"tr_123", 5};

    OrchestrationResult result = orchestrate_epoch(ctx, &metadata, &constraints, &transcript, "state_root_104");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    EXPECT_EQ(result.anchor_payload.epoch_id, 104);
    EXPECT_EQ(result.anchor_payload.state_root_hash, "state_root_104");
    EXPECT_EQ(result.anchor_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.proof_id, "");
    EXPECT_EQ(result.anchor_payload.zk_metadata.validation_status, DeterministicZKStatus::CONSTRAINT_MISMATCH);
}

} // namespace ailee::l6::tests
