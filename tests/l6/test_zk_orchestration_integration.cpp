#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"
#include "l6/ZKMockBackend.h"

using namespace ailee::l6;


class OrchestrationIntegrationTest : public ::testing::Test {
protected:
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};
    std::string state_root_hash = "deadbeef";

    OrchestrationContext get_context(AnchorDecision anchor_decision, ProofDecision proof_decision) {
        OrchestrationContext ctx;
        ctx.epoch_id = 42;
        ctx.anchor_plan.decision = anchor_decision;
        ctx.proof_plan.decision = proof_decision;
        return ctx;
    }
};

TEST_F(OrchestrationIntegrationTest, ProofGeneratedWhenSchedulingSaysAttach) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    auto result = orchestrate_epoch(ctx, &backend, config, &constraints, &transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.epoch_id, 42);
    EXPECT_EQ(payload.state_root_hash, "deadbeef");
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "constraint_1");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "transcript_1");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);

    // Proof commitment hash should not be empty and should match expected hash length
    EXPECT_FALSE(payload.proof_commitment_hash.empty());
    EXPECT_EQ(payload.proof_commitment_hash.length(), 64);
}

TEST_F(OrchestrationIntegrationTest, ProofSkippedWhenSchedulingSaysSkip) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::SKIP);

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

TEST_F(OrchestrationIntegrationTest, MissingBackendReturnsEmptyMetadata) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    // Pass nullptr as backend
    auto result = orchestrate_epoch(ctx, nullptr, config, &constraints, &transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof); // Scheduling still wants to attach, but no backend available

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

TEST_F(OrchestrationIntegrationTest, EmptyConstraintsEmptyTranscriptDeterministicHandling) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    ZKConstraintSet empty_constraints{"", 0};
    ZKTranscript empty_transcript{"", 0};

    auto result = orchestrate_epoch(ctx, &backend, config, &empty_constraints, &empty_transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::EMPTY_CONSTRAINTS);
    EXPECT_TRUE(payload.proof_commitment_hash.empty());
}

TEST_F(OrchestrationIntegrationTest, NullConstraintsHandledProperly) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    auto result = orchestrate_epoch(ctx, &backend, config, nullptr, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::EMPTY_CONSTRAINTS);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

TEST_F(OrchestrationIntegrationTest, NullTranscriptHandledProperly) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    auto result = orchestrate_epoch(ctx, &backend, config, &constraints, nullptr, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::EMPTY_TRANSCRIPT);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

// A simple mock backend wrapper that always fails verification to test failure paths
class AlwaysFailsVerifyBackend : public ZKMockBackend {
public:
    bool verify_proof(const ZKBackendConfig&, const ZKProofArtifact&, const ZKConstraintSet&, const ZKTranscript&) override {
        return false;
    }
};

TEST_F(OrchestrationIntegrationTest, VerificationFailureEmptyMetadata) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);
    AlwaysFailsVerifyBackend failing_backend;

    auto result = orchestrate_epoch(ctx, &failing_backend, config, &constraints, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::CONSTRAINT_MISMATCH);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}
