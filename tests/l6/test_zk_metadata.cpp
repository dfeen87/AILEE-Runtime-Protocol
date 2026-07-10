#include <gtest/gtest.h>
#include "l6/ZKMetadata.h"

namespace ailee::l6::tests {

TEST(ZKMetadataTest, ConstraintSetSerialization) {
    ZKConstraintSet cs{"constraints_123", 42};
    auto bytes = cs.to_bytes();

    // id length: 15 ("constraints_123")
    // num_constraints: 8 bytes
    EXPECT_EQ(bytes.size(), 15 + 8);

    std::string id_str(bytes.begin(), bytes.begin() + 15);
    EXPECT_EQ(id_str, "constraints_123");

    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(bytes[15 + i]) << (8 * i));
    }
    EXPECT_EQ(val, 42);
}

TEST(ZKMetadataTest, TranscriptSerialization) {
    ZKTranscript tr{"transcript_xyz", 100};
    auto bytes = tr.to_bytes();

    EXPECT_EQ(bytes.size(), 14 + 8);

    std::string id_str(bytes.begin(), bytes.begin() + 14);
    EXPECT_EQ(id_str, "transcript_xyz");

    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(bytes[14 + i]) << (8 * i));
    }
    EXPECT_EQ(val, 100);
}

TEST(ZKMetadataTest, ProofMetadataSerialization) {
    ZKProofMetadata meta{"proof_1", "cs_1", "tr_1", 2048};
    auto bytes = meta.to_bytes();

    // proof_id: 7 ("proof_1")
    // cs_id: 4 ("cs_1")
    // tr_id: 4 ("tr_1")
    // size: 8
    EXPECT_EQ(bytes.size(), 7 + 4 + 4 + 8);

    std::string p_id(bytes.begin(), bytes.begin() + 7);
    EXPECT_EQ(p_id, "proof_1");

    std::string c_id(bytes.begin() + 7, bytes.begin() + 11);
    EXPECT_EQ(c_id, "cs_1");

    std::string t_id(bytes.begin() + 11, bytes.begin() + 15);
    EXPECT_EQ(t_id, "tr_1");

    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(bytes[15 + i]) << (8 * i));
    }
    EXPECT_EQ(val, 2048);
}

} // namespace ailee::l6::tests
