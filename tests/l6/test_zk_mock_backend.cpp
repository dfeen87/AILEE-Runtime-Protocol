#include <gtest/gtest.h>
#include "l6/ZKMockBackend.h"
#include <iomanip>
#include <sstream>

using namespace ailee::l6;

namespace {
    std::string to_hex(const std::vector<uint8_t>& data) {
        std::string hex_chars = "0123456789abcdef";
        std::string hex;
        hex.reserve(data.size() * 2);
        for (uint8_t b : data) {
            hex.push_back(hex_chars[b >> 4]);
            hex.push_back(hex_chars[b & 0x0F]);
        }
        return hex;
    }
}

TEST(ZKMockBackendTest, DeterministicProofGenerationForIdenticalInputs) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact1 = backend.generate_proof(config, constraints, transcript);
    auto artifact2 = backend.generate_proof(config, constraints, transcript);

    EXPECT_EQ(artifact1.proof_bytes, artifact2.proof_bytes);
    EXPECT_EQ(artifact1.metadata.proof_id, artifact2.metadata.proof_id);
}

TEST(ZKMockBackendTest, DifferentConstraintsProduceDifferentProofBytes) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};

    ZKConstraintSet constraints1{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};
    auto artifact1 = backend.generate_proof(config, constraints1, transcript);

    ZKConstraintSet constraints2{"constraint_2", 100}; // Different constraints
    auto artifact2 = backend.generate_proof(config, constraints2, transcript);

    EXPECT_NE(artifact1.proof_bytes, artifact2.proof_bytes);
    EXPECT_NE(artifact1.metadata.proof_id, artifact2.metadata.proof_id);
}

TEST(ZKMockBackendTest, DifferentTranscriptProduceDifferentProofBytes) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};

    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript1{"transcript_1", 10};
    auto artifact1 = backend.generate_proof(config, constraints, transcript1);

    ZKTranscript transcript2{"transcript_2", 15}; // Different transcript
    auto artifact2 = backend.generate_proof(config, constraints, transcript2);

    EXPECT_NE(artifact1.proof_bytes, artifact2.proof_bytes);
    EXPECT_NE(artifact1.metadata.proof_id, artifact2.metadata.proof_id);
}

TEST(ZKMockBackendTest, DifferentCircuitIdProduceDifferentProofBytes) {
    ZKMockBackend backend;

    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    ZKBackendConfig config1{ZKBackendType::HALO2, "mock_circuit_v23"};
    auto artifact1 = backend.generate_proof(config1, constraints, transcript);

    ZKBackendConfig config2{ZKBackendType::HALO2, "different_circuit_v23"};
    auto artifact2 = backend.generate_proof(config2, constraints, transcript);

    EXPECT_NE(artifact1.proof_bytes, artifact2.proof_bytes);
    EXPECT_NE(artifact1.metadata.proof_id, artifact2.metadata.proof_id);
}

TEST(ZKMockBackendTest, VerifyProofReturnsTrueForMatchingInputs) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend.generate_proof(config, constraints, transcript);

    EXPECT_TRUE(backend.verify_proof(config, artifact, constraints, transcript));
}

TEST(ZKMockBackendTest, VerifyProofReturnsFalseForMismatchedInputs) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend.generate_proof(config, constraints, transcript);

    ZKConstraintSet mismatched_constraints{"constraint_2", 100};
    EXPECT_FALSE(backend.verify_proof(config, artifact, mismatched_constraints, transcript));

    ZKTranscript mismatched_transcript{"transcript_2", 10};
    EXPECT_FALSE(backend.verify_proof(config, artifact, constraints, mismatched_transcript));

    ZKBackendConfig mismatched_config{ZKBackendType::HALO2, "wrong_circuit"};
    EXPECT_FALSE(backend.verify_proof(mismatched_config, artifact, constraints, transcript));

    ZKProofArtifact mismatched_artifact = artifact;
    if (!mismatched_artifact.proof_bytes.empty()) {
        mismatched_artifact.proof_bytes[0] ^= 0xFF; // Modify a byte
    }
    EXPECT_FALSE(backend.verify_proof(config, mismatched_artifact, constraints, transcript));
}

TEST(ZKMockBackendTest, MetadataFieldsMatchExpectedValues) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend.generate_proof(config, constraints, transcript);

    EXPECT_EQ(artifact.metadata.constraint_set_id, constraints.id);
    EXPECT_EQ(artifact.metadata.transcript_id, transcript.id);
    EXPECT_EQ(artifact.metadata.logical_size_bytes, 32);
    EXPECT_EQ(artifact.proof_bytes.size(), 32);
}

TEST(ZKMockBackendTest, ProofIdMatchesHexEncodedDigest) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend.generate_proof(config, constraints, transcript);

    std::string expected_hex = to_hex(artifact.proof_bytes);
    EXPECT_EQ(artifact.metadata.proof_id, expected_hex);
    EXPECT_EQ(artifact.metadata.proof_id.length(), 64);
}

TEST(ZKMockBackendTest, EmptyInputsHashProperly) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet empty_constraints{"", 0};
    ZKTranscript empty_transcript{"", 0};

    auto artifact = backend.generate_proof(config, empty_constraints, empty_transcript);

    EXPECT_EQ(artifact.proof_bytes.size(), 32);
    EXPECT_EQ(artifact.metadata.proof_id.length(), 64);
    EXPECT_EQ(artifact.metadata.logical_size_bytes, 32);

    EXPECT_TRUE(backend.verify_proof(config, artifact, empty_constraints, empty_transcript));
}
