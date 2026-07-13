#include <gtest/gtest.h>
#include "l6/ZKMetadata.h"
#include "l6/ZKMockBackend.h"
#include "l6/ZKRecursionManager.h"
#include "l6/ZKProvingBackend.h"

using namespace ailee::l6;

class ZKRecursionLayerTest : public ::testing::Test {
public:
    ZKMockBackend backend;
    ZKRecursionManager recursion_manager;
    ZKBackendConfig config;

    void SetUp() override {
        config.type = ZKBackendType::MOCK;
        config.circuit_id = "test_recursion_circuit_v33";
        config.circuit_path = "";
        config.proving_key_path = "";
        config.verification_key_path = "";
    }
};

TEST_F(ZKRecursionLayerTest, ThrowsOnEmptyPreviousProof) {
    ZKConstraintSet constraints{"c_set_1", 100};
    ZKTranscript transcript{"t_1", 5};
    ZKRecursionBundle empty_bundle;

    bool threw = false;
    try {
        backend.generate_recursive_proof(config, constraints, transcript, empty_bundle);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST_F(ZKRecursionLayerTest, GeneratesAndVerifiesRecursiveProof) {
    ZKConstraintSet constraints_prev{"c_prev", 50};
    ZKTranscript transcript_prev{"t_prev", 2};
    ZKProofArtifact prev_proof = backend.generate_proof(config, constraints_prev, transcript_prev);

    ZKRecursionBundle bundle;
    bundle.previous_proof_artifact = prev_proof;
    bundle.previous_state_root = "deadbeef";
    bundle.previous_epoch_id = 42;

    ZKConstraintSet constraints_curr{"c_curr", 100};
    ZKTranscript transcript_curr{"t_curr", 5};

    ZKProofArtifact rec_proof = backend.generate_recursive_proof(config, constraints_curr, transcript_curr, bundle);

    EXPECT_FALSE(rec_proof.proof_bytes.empty());
    EXPECT_FALSE(rec_proof.metadata.proof_id.empty());
    EXPECT_EQ(rec_proof.metadata.constraint_set_id, "c_curr");
    EXPECT_EQ(rec_proof.metadata.transcript_id, "t_curr");

    bool is_valid = backend.verify_recursive_proof(config, rec_proof, constraints_curr, transcript_curr, bundle);
    EXPECT_TRUE(is_valid);
}

TEST_F(ZKRecursionLayerTest, DeterministicProofChangesWithPreviousProof) {
    ZKConstraintSet constraints_curr{"c_curr", 100};
    ZKTranscript transcript_curr{"t_curr", 5};

    ZKProofArtifact dummy_prev1;
    dummy_prev1.proof_bytes = {0x01, 0x02, 0x03};
    dummy_prev1.metadata.proof_id = "dummy1";

    ZKRecursionBundle bundle1;
    bundle1.previous_proof_artifact = dummy_prev1;
    bundle1.previous_state_root = "deadbeef";
    bundle1.previous_epoch_id = 42;

    ZKProofArtifact rec_proof1 = backend.generate_recursive_proof(config, constraints_curr, transcript_curr, bundle1);

    ZKProofArtifact dummy_prev2;
    dummy_prev2.proof_bytes = {0x01, 0x02, 0x04}; // Changed one byte
    dummy_prev2.metadata.proof_id = "dummy2";

    ZKRecursionBundle bundle2;
    bundle2.previous_proof_artifact = dummy_prev2;
    bundle2.previous_state_root = "deadbeef";
    bundle2.previous_epoch_id = 42;

    ZKProofArtifact rec_proof2 = backend.generate_recursive_proof(config, constraints_curr, transcript_curr, bundle2);

    EXPECT_NE(rec_proof1.proof_bytes, rec_proof2.proof_bytes);
    EXPECT_NE(rec_proof1.metadata.proof_id, rec_proof2.metadata.proof_id);
}

TEST_F(ZKRecursionLayerTest, RecursionManagerValidatesContinuity) {
    ZKProofArtifact dummy_prev;
    dummy_prev.proof_bytes = {0x01};

    bool threw1 = false;
    try {
        recursion_manager.build_recursion_bundle(dummy_prev, "root_a", 10, "root_a", 11);
    } catch (...) {
        threw1 = true;
    }
    EXPECT_FALSE(threw1);

    bool threw2 = false;
    try {
        recursion_manager.build_recursion_bundle(dummy_prev, "root_a", 10, "root_a", 12);
    } catch (const std::invalid_argument&) {
        threw2 = true;
    }
    EXPECT_TRUE(threw2);

    bool threw3 = false;
    try {
        recursion_manager.build_recursion_bundle(dummy_prev, "root_a", 10, "root_b", 11);
    } catch (const std::invalid_argument&) {
        threw3 = true;
    }
    EXPECT_TRUE(threw3);
}
