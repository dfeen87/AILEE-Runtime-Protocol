#include <gtest/gtest.h>
#include "l6/ZKBackendFactory.h"
#include "l6/ZKMockBackend.h"
#include "l6/Halo2Backend.h"
#include "l6/PlonkBackend.h"

using namespace ailee::l6;

TEST(ZKBackendActivationTest, FactoryCreatesMockBackend) {
    ZKBackendConfig config{ZKBackendType::MOCK, "test_circuit"};
    auto backend = make_backend(config);
    EXPECT_NE(dynamic_cast<ZKMockBackend*>(backend.get()), nullptr);
}

TEST(ZKBackendActivationTest, FactoryCreatesHalo2Backend) {
    ZKBackendConfig config{ZKBackendType::HALO2_NATIVE, "test_circuit"};
    auto backend = make_backend(config);
    EXPECT_NE(dynamic_cast<Halo2Backend*>(backend.get()), nullptr);
}

TEST(ZKBackendActivationTest, FactoryCreatesPlonkBackend) {
    ZKBackendConfig config{ZKBackendType::PLONK_NATIVE, "test_circuit"};
    auto backend = make_backend(config);
    EXPECT_NE(dynamic_cast<PlonkBackend*>(backend.get()), nullptr);
}

TEST(ZKBackendActivationTest, Halo2BackendIsDeterministic) {
    ZKBackendConfig config{ZKBackendType::HALO2_NATIVE, "test_circuit"};
    Halo2Backend backend;
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact1 = backend.generate_proof(config, constraints, transcript);
    auto artifact2 = backend.generate_proof(config, constraints, transcript);

    EXPECT_EQ(artifact1.proof_bytes, artifact2.proof_bytes);
    EXPECT_EQ(artifact1.metadata.proof_id, artifact2.metadata.proof_id);
    EXPECT_EQ(artifact1.metadata.constraint_set_id, "constraint_1");
    EXPECT_EQ(artifact1.metadata.transcript_id, "transcript_1");

    EXPECT_TRUE(backend.verify_proof(config, artifact1, constraints, transcript));
}

TEST(ZKBackendActivationTest, PlonkBackendIsDeterministic) {
    ZKBackendConfig config{ZKBackendType::PLONK_NATIVE, "test_circuit"};
    PlonkBackend backend;
    ZKConstraintSet constraints{"constraint_1", 100};
    ZKTranscript transcript{"transcript_1", 10};

    auto artifact1 = backend.generate_proof(config, constraints, transcript);
    auto artifact2 = backend.generate_proof(config, constraints, transcript);

    EXPECT_EQ(artifact1.proof_bytes, artifact2.proof_bytes);
    EXPECT_EQ(artifact1.metadata.proof_id, artifact2.metadata.proof_id);
    EXPECT_EQ(artifact1.metadata.constraint_set_id, "constraint_1");
    EXPECT_EQ(artifact1.metadata.transcript_id, "transcript_1");

    EXPECT_TRUE(backend.verify_proof(config, artifact1, constraints, transcript));
}
