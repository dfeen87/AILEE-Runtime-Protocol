#include <gtest/gtest.h>
#include "l6/ZKMockBackend.h"

using namespace ailee::l6;

TEST(ZKMockBackendTest, GenerateAndVerifyProof) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend.generate_proof(config, constraints, transcript);

    // Check metadata
    EXPECT_EQ(artifact.metadata.constraint_set_id, constraints.id);
    EXPECT_EQ(artifact.metadata.transcript_id, transcript.id);
    EXPECT_FALSE(artifact.metadata.proof_id.empty());
    EXPECT_EQ(artifact.metadata.logical_size_bytes, artifact.proof_bytes.size());
    EXPECT_FALSE(artifact.proof_bytes.empty());

    // Verification should succeed
    EXPECT_TRUE(backend.verify_proof(config, artifact, constraints, transcript));
}

TEST(ZKMockBackendTest, VerificationFailsOnMismatch) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend.generate_proof(config, constraints, transcript);

    // Modify transcript ID to simulate mismatch
    ZKTranscript bad_transcript{"transcript_2", 10};
    EXPECT_FALSE(backend.verify_proof(config, artifact, constraints, bad_transcript));

    // Modify constraint ID to simulate mismatch
    ZKConstraintSet bad_constraints{"constraint_2", 100};
    EXPECT_FALSE(backend.verify_proof(config, artifact, bad_constraints, transcript));
}

TEST(ZKMockBackendTest, DeterministicGeneration) {
    ZKMockBackend backend;

    ZKBackendConfig config{ZKBackendType::HALO2, "test_circuit"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact1 = backend.generate_proof(config, constraints, transcript);
    auto artifact2 = backend.generate_proof(config, constraints, transcript);

    EXPECT_EQ(artifact1.proof_bytes, artifact2.proof_bytes);
    EXPECT_EQ(artifact1.metadata.proof_id, artifact2.metadata.proof_id);
}
