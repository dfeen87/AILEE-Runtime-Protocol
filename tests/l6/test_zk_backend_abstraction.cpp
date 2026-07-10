#include <gtest/gtest.h>
#include "l6/ZKProvingBackend.h"
#include "l6/ZKMockBackend.h"
#include <memory>

using namespace ailee::l6;

TEST(ZKBackendAbstractionTest, BackendConfigDeterminism) {
    ZKBackendConfig config1{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKBackendConfig config2{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKBackendConfig config3{ZKBackendType::PLONK, "mock_circuit_v23"};
    ZKBackendConfig config4{ZKBackendType::HALO2, "another_circuit"};

    EXPECT_EQ(config1.type, config2.type);
    EXPECT_EQ(config1.circuit_id, config2.circuit_id);

    EXPECT_NE(config1.type, config3.type);
    EXPECT_NE(config1.circuit_id, config4.circuit_id);
}

TEST(ZKBackendAbstractionTest, ArtifactStructureCorrectness) {
    ZKMockBackend backend;
    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend.generate_proof(config, constraints, transcript);

    EXPECT_FALSE(artifact.proof_bytes.empty());
    EXPECT_FALSE(artifact.metadata.proof_id.empty());
    EXPECT_EQ(artifact.metadata.constraint_set_id, "constraint_1");
    EXPECT_EQ(artifact.metadata.transcript_id, "transcript_1");
    EXPECT_EQ(artifact.metadata.logical_size_bytes, artifact.proof_bytes.size());
}

TEST(ZKBackendAbstractionTest, BackendInterfaceCompliance) {
    // This test ensures that ZKMockBackend can be used purely through its IZKProvingBackend interface
    std::unique_ptr<IZKProvingBackend> backend = std::make_unique<ZKMockBackend>();

    ZKBackendConfig config{ZKBackendType::HALO2, "mock_circuit_v23"};
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact = backend->generate_proof(config, constraints, transcript);
    EXPECT_FALSE(artifact.proof_bytes.empty());
    EXPECT_TRUE(backend->verify_proof(config, artifact, constraints, transcript));
}
