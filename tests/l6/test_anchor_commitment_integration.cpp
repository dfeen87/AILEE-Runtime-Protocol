#include <gtest/gtest.h>
#include "l6/ZKOrchestrationManager.h"
#include "l6/ZKMockBackend.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

using namespace ailee::l6;

TEST(AnchorCommitmentIntegrationTest, ValidProofHasNonEmpty64CharHexCommitment) {
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
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
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);

    // commitment should be a 64 char hex string
    EXPECT_EQ(payload.proof_commitment_hash.length(), 64);
    for (char c : payload.proof_commitment_hash) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

TEST(AnchorCommitmentIntegrationTest, DifferentProofBytesProduceDifferentCommitments) {
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};

    OrchestrationContext ctx;
    ctx.epoch_id = 42;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;
    std::string state_root_hash = "deadbeef";

    ZKConstraintSet constraints1{"constraint_1", 100};
    ZKTranscript transcript1{"transcript_1", 10};
    auto result1 = orchestrate_epoch(ctx, &backend, config, &constraints1, &transcript1, state_root_hash);

    ZKConstraintSet constraints2{"constraint_2", 100};
    ZKTranscript transcript2{"transcript_2", 10};
    auto result2 = orchestrate_epoch(ctx, &backend, config, &constraints2, &transcript2, state_root_hash);

    EXPECT_NE(result1.anchor_payload.proof_commitment_hash, result2.anchor_payload.proof_commitment_hash);
}

TEST(AnchorCommitmentIntegrationTest, SkippedProofProducesEmptyCommitment) {
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    OrchestrationContext ctx;
    ctx.epoch_id = 42;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::SKIP; // SKIP

    std::string state_root_hash = "deadbeef";

    auto result = orchestrate_epoch(ctx, &backend, config, &constraints, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

TEST(AnchorCommitmentIntegrationTest, FailedValidationProducesEmptyCommitment) {
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};

    // Using mismatched constraints during orchestration simulation isn't straightforward without a custom backend
    // unless we pass null for constraints or transcript (which the orchestrator checks)
    // Wait, the validation logic in ZKOrchestrationManager checks if constraints are null

    OrchestrationContext ctx;
    ctx.epoch_id = 42;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;
    std::string state_root_hash = "deadbeef";

    ZKTranscript transcript{"transcript_1", 10};
    auto result = orchestrate_epoch(ctx, &backend, config, nullptr, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::EMPTY_CONSTRAINTS);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

// Custom mock backend to force verification failure
class FailingZKMockBackend : public IZKProvingBackend {
public:
    ZKProofArtifact generate_proof(const ZKBackendConfig& config, const ZKConstraintSet& constraints, const ZKTranscript& transcript) override {
        ZKMockBackend real_mock;
        return real_mock.generate_proof(config, constraints, transcript);
    }

    bool verify_proof(const ZKBackendConfig& config, const ZKProofArtifact& artifact, const ZKConstraintSet& constraints, const ZKTranscript& transcript) override {
        return false; // Force failure
    }
};

TEST(AnchorCommitmentIntegrationTest, FailedVerificationProducesEmptyCommitment) {
    FailingZKMockBackend failing_backend;
    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    OrchestrationContext ctx;
    ctx.epoch_id = 42;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    std::string state_root_hash = "deadbeef";

    auto result = orchestrate_epoch(ctx, &failing_backend, config, &constraints, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    // When verification fails but validation succeeds, it maps to CONSTRAINT_MISMATCH in orchestrate_epoch
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::CONSTRAINT_MISMATCH);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

TEST(AnchorCommitmentIntegrationTest, NullBackendProducesEmptyCommitment) {
    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    OrchestrationContext ctx;
    ctx.epoch_id = 42;
    ctx.anchor_plan.decision = AnchorDecision::ANCHOR;
    ctx.proof_plan.decision = ProofDecision::ATTACH_PROOF;

    std::string state_root_hash = "deadbeef";

    auto result = orchestrate_epoch(ctx, nullptr, config, &constraints, &transcript, state_root_hash);

    auto& payload = result.anchor_payload;
    EXPECT_EQ(payload.zk_metadata.validation_status, DeterministicZKStatus::OK);
    EXPECT_EQ(payload.proof_commitment_hash, "");
}

TEST(AnchorCommitmentIntegrationTest, AnchorPayloadToBytesIncludesCommitmentDeterministically) {
    AnchorPayload payload1;
    payload1.epoch_id = 1;
    payload1.state_root_hash = "state_1";
    payload1.zk_metadata.proof_id = "proof_1";
    payload1.zk_metadata.constraint_set_id = "constraint_1";
    payload1.zk_metadata.transcript_id = "transcript_1";
    payload1.zk_metadata.validation_status = DeterministicZKStatus::OK;
    payload1.proof_commitment_hash = "abc123def456";

    std::vector<uint8_t> bytes1 = payload1.to_bytes();

    AnchorPayload payload2 = payload1;
    payload2.proof_commitment_hash = "987xyz654uvw";

    std::vector<uint8_t> bytes2 = payload2.to_bytes();

    EXPECT_NE(bytes1, bytes2);

    // Because proof_commitment_hash is the last appended, bytes2 and bytes1
    // should have the same prefix and differ only at the end.
    // the length should be the same since the hashes are same length
    EXPECT_EQ(bytes1.size(), bytes2.size());

    // Check that we can find the appended strings in the raw bytes for the test
    std::string b1_str(bytes1.begin(), bytes1.end());
    std::string b2_str(bytes2.begin(), bytes2.end());

    EXPECT_TRUE(b1_str.find("abc123def456") != std::string::npos);
    EXPECT_TRUE(b2_str.find("987xyz654uvw") != std::string::npos);
}

TEST(AnchorCommitmentIntegrationTest, AnchorPayloadReplayConsistencyAndMismatchDetection) {
    AnchorPayload original_payload;
    original_payload.epoch_id = 42;
    original_payload.state_root_hash = "deadbeef";
    original_payload.zk_metadata.proof_id = "proof_1";
    original_payload.zk_metadata.constraint_set_id = "constraint_1";
    original_payload.zk_metadata.transcript_id = "transcript_1";
    original_payload.zk_metadata.validation_status = DeterministicZKStatus::OK;
    original_payload.proof_commitment_hash = "abc123def456";

    std::vector<uint8_t> original_bytes = original_payload.to_bytes();

    AnchorPayload replayed_payload = original_payload;
    std::vector<uint8_t> replayed_bytes = replayed_payload.to_bytes();

    EXPECT_EQ(original_bytes, replayed_bytes);

    // Mismatch detection
    AnchorPayload modified_payload = original_payload;
    modified_payload.state_root_hash = "badf00d";
    std::vector<uint8_t> modified_bytes = modified_payload.to_bytes();
    EXPECT_NE(original_bytes, modified_bytes);

    AnchorPayload modified_metadata = original_payload;
    modified_metadata.zk_metadata.validation_status = DeterministicZKStatus::CONSTRAINT_MISMATCH;
    EXPECT_NE(original_bytes, modified_metadata.to_bytes());
}
