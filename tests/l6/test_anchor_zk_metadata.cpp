#include <gtest/gtest.h>
#include "l6/AnchorZKMetadata.h"
#include "l6/ProofScheduling.h"

namespace ailee::l6::tests {

TEST(AnchorZKMetadataTest, SkipPlanResultsInEmptyMetadata) {
    AnchorPayload base_payload;
    base_payload.epoch_id = 42;
    base_payload.state_root_hash = "abcdef";
    base_payload.zk_metadata.constraint_set_id = "initial";
    base_payload.zk_metadata.transcript_id = "initial";
    base_payload.zk_metadata.proof_id = "initial";
    base_payload.zk_metadata.validation_status = DeterministicZKStatus::TRANSCRIPT_MISMATCH;

    ProofPlan proof_plan;
    proof_plan.decision = ProofDecision::SKIP;
    proof_plan.epoch_id = 42;
    proof_plan.proof_cycle_id = 4;

    ZKProofMetadata metadata;
    metadata.proof_id = "proof_123";
    metadata.constraint_set_id = "constraints_abc";
    metadata.transcript_id = "transcript_xyz";
    metadata.logical_size_bytes = 1024;

    ZKConstraintSet constraints{"constraints_abc", 10};
    ZKTranscript transcript{"transcript_xyz", 5};

    AnchorPayload new_payload = attach_zk_to_anchor(base_payload, proof_plan, &metadata, &constraints, &transcript);

    EXPECT_EQ(new_payload.epoch_id, 42);
    EXPECT_EQ(new_payload.state_root_hash, "abcdef");
    EXPECT_EQ(new_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(new_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(new_payload.zk_metadata.proof_id, "");
    EXPECT_EQ(new_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
}

TEST(AnchorZKMetadataTest, AttachProofWithValidMetadataPopulatesMetadata) {
    AnchorPayload base_payload;
    base_payload.epoch_id = 42;
    base_payload.state_root_hash = "abcdef";

    ProofPlan proof_plan;
    proof_plan.decision = ProofDecision::ATTACH_PROOF;
    proof_plan.epoch_id = 42;
    proof_plan.proof_cycle_id = 4;

    ZKProofMetadata metadata;
    metadata.proof_id = "proof_123";
    metadata.constraint_set_id = "constraints_abc";
    metadata.transcript_id = "transcript_xyz";
    metadata.logical_size_bytes = 1024;

    ZKConstraintSet constraints{"constraints_abc", 10};
    ZKTranscript transcript{"transcript_xyz", 5};

    AnchorPayload new_payload = attach_zk_to_anchor(base_payload, proof_plan, &metadata, &constraints, &transcript);

    EXPECT_EQ(new_payload.epoch_id, 42);
    EXPECT_EQ(new_payload.state_root_hash, "abcdef");
    EXPECT_EQ(new_payload.zk_metadata.constraint_set_id, "constraints_abc");
    EXPECT_EQ(new_payload.zk_metadata.transcript_id, "transcript_xyz");
    EXPECT_EQ(new_payload.zk_metadata.proof_id, "proof_123");
    EXPECT_EQ(new_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
}

TEST(AnchorZKMetadataTest, AttachProofWithNullptrMetadataResultsInEmptyMetadata) {
    AnchorPayload base_payload;
    base_payload.epoch_id = 42;
    base_payload.state_root_hash = "abcdef";

    ProofPlan proof_plan;
    proof_plan.decision = ProofDecision::ATTACH_PROOF;
    proof_plan.epoch_id = 42;
    proof_plan.proof_cycle_id = 4;

    AnchorPayload new_payload = attach_zk_to_anchor(base_payload, proof_plan, nullptr, nullptr, nullptr);

    EXPECT_EQ(new_payload.epoch_id, 42);
    EXPECT_EQ(new_payload.state_root_hash, "abcdef");
    EXPECT_EQ(new_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(new_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(new_payload.zk_metadata.proof_id, "");
    EXPECT_EQ(new_payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
}

TEST(AnchorZKMetadataTest, AttachProofWithInvalidMetadataResultsInFailureStatus) {
    AnchorPayload base_payload;
    base_payload.epoch_id = 42;
    base_payload.state_root_hash = "abcdef";

    ProofPlan proof_plan;
    proof_plan.decision = ProofDecision::ATTACH_PROOF;
    proof_plan.epoch_id = 42;
    proof_plan.proof_cycle_id = 4;

    ZKProofMetadata metadata;
    metadata.proof_id = "proof_123";
    metadata.constraint_set_id = "constraints_wrong";
    metadata.transcript_id = "transcript_xyz";
    metadata.logical_size_bytes = 1024;

    ZKConstraintSet constraints{"constraints_abc", 10};
    ZKTranscript transcript{"transcript_xyz", 5};

    AnchorPayload new_payload = attach_zk_to_anchor(base_payload, proof_plan, &metadata, &constraints, &transcript);

    EXPECT_EQ(new_payload.epoch_id, 42);
    EXPECT_EQ(new_payload.state_root_hash, "abcdef");
    EXPECT_EQ(new_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(new_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(new_payload.zk_metadata.proof_id, "");
    EXPECT_EQ(new_payload.zk_metadata.validation_status, DeterministicZKStatus::CONSTRAINT_MISMATCH);
}

} // namespace ailee::l6::tests
