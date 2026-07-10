#include <gtest/gtest.h>
#include "l6/AnchorZKMetadata.h"
#include "l6/ProofScheduling.h"
#include "l6/ZKStubInterface.h"

namespace ailee::l6::tests {

TEST(AnchorZKMetadataTest, SkipPlanResultsInEmptyMetadata) {
    AnchorPayload base_payload;
    base_payload.epoch_id = 42;
    base_payload.state_root_hash = "abcdef";
    base_payload.zk_metadata.constraint_set_id = "initial";
    base_payload.zk_metadata.transcript_id = "initial";
    base_payload.zk_metadata.proof_id = "initial";

    ProofPlan proof_plan;
    proof_plan.decision = ProofDecision::SKIP;
    proof_plan.epoch_id = 42;
    proof_plan.proof_cycle_id = 4;

    ZKProofStub stub;
    stub.proof_id = "proof_123";
    stub.constraint_set_id = "constraints_abc";
    stub.transcript_id = "transcript_xyz";
    stub.size_bytes = 1024;

    AnchorPayload new_payload = attach_zk_to_anchor(base_payload, proof_plan, &stub);

    EXPECT_EQ(new_payload.epoch_id, 42);
    EXPECT_EQ(new_payload.state_root_hash, "abcdef");
    EXPECT_EQ(new_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(new_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(new_payload.zk_metadata.proof_id, "");
}

TEST(AnchorZKMetadataTest, AttachProofWithValidStubPopulatesMetadata) {
    AnchorPayload base_payload;
    base_payload.epoch_id = 42;
    base_payload.state_root_hash = "abcdef";

    ProofPlan proof_plan;
    proof_plan.decision = ProofDecision::ATTACH_PROOF;
    proof_plan.epoch_id = 42;
    proof_plan.proof_cycle_id = 4;

    ZKProofStub stub;
    stub.proof_id = "proof_123";
    stub.constraint_set_id = "constraints_abc";
    stub.transcript_id = "transcript_xyz";
    stub.size_bytes = 1024;

    AnchorPayload new_payload = attach_zk_to_anchor(base_payload, proof_plan, &stub);

    EXPECT_EQ(new_payload.epoch_id, 42);
    EXPECT_EQ(new_payload.state_root_hash, "abcdef");
    EXPECT_EQ(new_payload.zk_metadata.constraint_set_id, "constraints_abc");
    EXPECT_EQ(new_payload.zk_metadata.transcript_id, "transcript_xyz");
    EXPECT_EQ(new_payload.zk_metadata.proof_id, "proof_123");
}

TEST(AnchorZKMetadataTest, AttachProofWithNullptrStubResultsInEmptyMetadata) {
    AnchorPayload base_payload;
    base_payload.epoch_id = 42;
    base_payload.state_root_hash = "abcdef";

    ProofPlan proof_plan;
    proof_plan.decision = ProofDecision::ATTACH_PROOF;
    proof_plan.epoch_id = 42;
    proof_plan.proof_cycle_id = 4;

    AnchorPayload new_payload = attach_zk_to_anchor(base_payload, proof_plan, nullptr);

    EXPECT_EQ(new_payload.epoch_id, 42);
    EXPECT_EQ(new_payload.state_root_hash, "abcdef");
    EXPECT_EQ(new_payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(new_payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(new_payload.zk_metadata.proof_id, "");
}

} // namespace ailee::l6::tests
