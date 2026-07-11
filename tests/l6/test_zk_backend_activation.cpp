#include <gtest/gtest.h>
#include "l6/ZKBackendFactory.h"
#include "l6/ZKMockBackend.h"
#include "l6/Halo2Backend.h"
#include "l6/PlonkBackend.h"
#include "l6/IslaRuntimeOrchestrator.h"
#include "l6/RuntimeEnvironment.h"

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

TEST(ZKBackendActivationTest, IslaThrowsOnInvalidActivationCI) {
    RuntimeEnvironment env;
    env.is_ci = true;
    IslaRuntimeOrchestrator isla(env);

    ZKBackendConfig halo2_config{ZKBackendType::HALO2_NATIVE, "test_circuit"};
    { bool threw_DeterministicBackendException = false; try { isla.attach_backend(halo2_config); } catch(const DeterministicBackendException&) { threw_DeterministicBackendException = true; } EXPECT_TRUE(threw_DeterministicBackendException); }

    ZKBackendConfig plonk_config{ZKBackendType::PLONK_NATIVE, "test_circuit"};
    { bool threw_DeterministicBackendException = false; try { isla.attach_backend(plonk_config); } catch(const DeterministicBackendException&) { threw_DeterministicBackendException = true; } EXPECT_TRUE(threw_DeterministicBackendException); }

    ZKBackendConfig mock_config{ZKBackendType::MOCK, "test_circuit"};
    { bool no_throw = true; try { isla.attach_backend(mock_config); } catch(...) { no_throw = false; } EXPECT_TRUE(no_throw); }
}

TEST(ZKBackendActivationTest, IslaThrowsOnInvalidActivationNonCI) {
    RuntimeEnvironment env;
    env.is_ci = false; // Maps to DEV environment in Semantics
    IslaRuntimeOrchestrator isla(env);

    // MOCK is allowed in DEV
    ZKBackendConfig mock_config{ZKBackendType::MOCK, "test_circuit"};
    { bool no_throw = true; try { isla.attach_backend(mock_config); } catch(...) { no_throw = false; } EXPECT_TRUE(no_throw); }

    ZKBackendConfig halo2_config{ZKBackendType::HALO2_NATIVE, "test_circuit"};
    { bool no_throw = true; try { isla.attach_backend(halo2_config); } catch(...) { no_throw = false; } EXPECT_TRUE(no_throw); }

    // PLONK is NOT allowed in DEV, only PROD
    ZKBackendConfig plonk_config{ZKBackendType::PLONK_NATIVE, "test_circuit"};
    { bool threw_DeterministicBackendException = false; try { isla.attach_backend(plonk_config); } catch(const DeterministicBackendException&) { threw_DeterministicBackendException = true; } EXPECT_TRUE(threw_DeterministicBackendException); }
}
