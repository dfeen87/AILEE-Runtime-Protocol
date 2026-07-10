#include <gtest/gtest.h>
#include "l6/ZKDeterministicValidator.h"

namespace ailee::l6::tests {

TEST(ZKValidationTest, ValidMetadata) {
    ZKProofMetadata proof{"proof_1", "cs_1", "tr_1", 1024};
    ZKConstraintSet constraints{"cs_1", 10};
    ZKTranscript transcript{"tr_1", 5};

    auto check = validate_zk_metadata(proof, constraints, transcript);
    EXPECT_EQ(check.status, DeterministicZKStatus::OK);
    EXPECT_EQ(check.proof_id, "proof_1");
}

TEST(ZKValidationTest, ConstraintMismatch) {
    ZKProofMetadata proof{"proof_1", "cs_wrong", "tr_1", 1024};
    ZKConstraintSet constraints{"cs_1", 10};
    ZKTranscript transcript{"tr_1", 5};

    auto check = validate_zk_metadata(proof, constraints, transcript);
    EXPECT_EQ(check.status, DeterministicZKStatus::CONSTRAINT_MISMATCH);
}

TEST(ZKValidationTest, TranscriptMismatch) {
    ZKProofMetadata proof{"proof_1", "cs_1", "tr_wrong", 1024};
    ZKConstraintSet constraints{"cs_1", 10};
    ZKTranscript transcript{"tr_1", 5};

    auto check = validate_zk_metadata(proof, constraints, transcript);
    EXPECT_EQ(check.status, DeterministicZKStatus::TRANSCRIPT_MISMATCH);
}

TEST(ZKValidationTest, EmptyConstraints) {
    ZKProofMetadata proof{"proof_1", "cs_1", "tr_1", 1024};
    ZKConstraintSet constraints{"cs_1", 0}; // 0 constraints
    ZKTranscript transcript{"tr_1", 5};

    auto check = validate_zk_metadata(proof, constraints, transcript);
    EXPECT_EQ(check.status, DeterministicZKStatus::EMPTY_CONSTRAINTS);
}

TEST(ZKValidationTest, EmptyTranscript) {
    ZKProofMetadata proof{"proof_1", "cs_1", "tr_1", 1024};
    ZKConstraintSet constraints{"cs_1", 10};
    ZKTranscript transcript{"tr_1", 0}; // 0 rounds

    auto check = validate_zk_metadata(proof, constraints, transcript);
    EXPECT_EQ(check.status, DeterministicZKStatus::EMPTY_TRANSCRIPT);
}

} // namespace ailee::l6::tests
