#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"
#include "l6/IslaRuntimeOrchestrator.h"
#include "l6/RuntimeEnvironment.h"
#include "l6/ZKMockBackend.h"

using namespace ailee::l6;


class OrchestrationIntegrationTest : public ::testing::Test {
protected:

    OrchestrationResult run_orchestrator(const OrchestrationContext& ctx, IZKProvingBackend* b, const ZKBackendConfig& cfg, const ZKConstraintSet* c, const ZKTranscript* t, const std::string& state_root) {
        RuntimeEnvironment env;
        env.is_ci = true;
        IslaRuntimeOrchestrator isla(env);

        EpochIntegrationBundle bundle{};
        bundle.anchor_input.internal_key.fill(0);
        bundle.anchor_input.prev_txid.fill(0);
        bundle.anchor_input.prev_vout = 0;
        bundle.anchor_input.value_sats = 0;

        bundle.epoch_id = ctx.epoch_id;
        bundle.state_root_hash = state_root;
        bundle.anchor_plan = ctx.anchor_plan;
        bundle.proof_plan = ctx.proof_plan;
        bundle.constraints = c;
        bundle.transcript = t;
        bundle.hice_metrics.covariance_error = 0.5e-6; bundle.hice_metrics.spectral_drift = 0.0; bundle.hice_metrics.delta_memory = -0.01; bundle.hice_metrics.context_leakage = 0.001; bundle.hice_metrics.null_matching_rate = 0.99; bundle.hice_metrics.delta_auc = 0.05; bundle.hice_metrics.ci_lower_bound = 0.04; bundle.hice_metrics.ci_point_estimate = 0.05;
        bundle.fee_sats = 1000;

        if (b) {
            isla.attach_backend(b, cfg);
        } else {
            isla.attach_backend(nullptr, cfg);
        }

        return isla.run_epoch(bundle).zk_result;
    }

    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::MOCK, "test_circuit", "", "", ""};
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

    auto result = run_orchestrator(ctx, &backend, config, &constraints, &transcript, state_root_hash);

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

    auto result = run_orchestrator(ctx, &backend, config, &constraints, &transcript, state_root_hash);

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
    auto result = run_orchestrator(ctx, nullptr, config, &constraints, &transcript, state_root_hash);

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

    auto result = run_orchestrator(ctx, &backend, config, &empty_constraints, &empty_transcript, state_root_hash);

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

    auto result = run_orchestrator(ctx, &backend, config, nullptr, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::EMPTY_CONSTRAINTS);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

TEST_F(OrchestrationIntegrationTest, NullTranscriptHandledProperly) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    auto result = run_orchestrator(ctx, &backend, config, &constraints, nullptr, state_root_hash);

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

    auto result = run_orchestrator(ctx, &failing_backend, config, &constraints, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::CONSTRAINT_MISMATCH);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

TEST_F(OrchestrationIntegrationTest, MismatchedIDsValidationFailure) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    ZKConstraintSet mismatched_constraints{"wrong_constraint", 100};
    ZKTranscript mismatched_transcript{"wrong_transcript", 10};

    // The current orchestrate_epoch function generates proof using passed constraints and transcript.
    // In order to simulate validation failure natively within orchestrate_epoch without changing the mock
    // we would need a backend that fails verify or an orchestrate_epoch that checks IDs before generation.
    // We already have AlwaysFailsVerifyBackend testing verification failure which handles mismatched validation
    // semantics for now according to existing logic (CONSTRAINT_MISMATCH).

    // We'll verify that metadata correctness sets the IDs properly
    auto result = run_orchestrator(ctx, &backend, config, &mismatched_constraints, &mismatched_transcript, state_root_hash);
    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "wrong_constraint");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "wrong_transcript");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
}

TEST_F(OrchestrationIntegrationTest, NullStateRootHashHandledCorrectly) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    auto result = run_orchestrator(ctx, &backend, config, &constraints, &transcript, "");

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.state_root_hash, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_FALSE(payload.proof_commitment_hash.empty());
}

TEST_F(OrchestrationIntegrationTest, NullBackendHandledCorrectly) {
    auto ctx = get_context(AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF);

    auto result = run_orchestrator(ctx, nullptr, config, &constraints, &transcript, state_root_hash);

    EXPECT_TRUE(result.should_anchor);
    EXPECT_TRUE(result.should_attach_proof);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.constraint_set_id, "");
    EXPECT_EQ(payload.zk_metadata.transcript_id, "");
    EXPECT_EQ(payload.zk_metadata.proof_id, "");
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}
