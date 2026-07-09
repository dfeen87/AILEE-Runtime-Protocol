#include "taproot/TaprootScript.h"
#include <gtest/gtest.h>
#include <cstring>
#include <iostream>

using namespace ailee::taproot;

TEST(AnchorCommitTapLeafTests, ExactScriptConstructionAndHash) {
    std::array<uint8_t, 32> dummy_anchor = {0};
    dummy_anchor[0] = 0xAA;
    dummy_anchor[31] = 0xAA;

    std::array<uint8_t, 32> dummy_meta = {0};
    dummy_meta[0] = 0xBB;
    dummy_meta[31] = 0xBB;

    auto tapleaf = TaprootScript::build_anchor_commit_tapleaf(dummy_anchor, dummy_meta);

    // script length: 1 (OP_PUSH32) + 32 + 1 (OP_DROP) + 1 (OP_PUSH32) + 32 + 1 (OP_DROP) + 1 (OP_TRUE) = 69
    EXPECT_EQ(tapleaf.script.size(), 69);

    // Check opcodes and data boundaries
    EXPECT_EQ(tapleaf.script[0], 0x20); // OP_PUSH32
    EXPECT_EQ(tapleaf.script[1], 0xAA); // anchor start
    EXPECT_EQ(tapleaf.script[32], 0xAA); // anchor end
    EXPECT_EQ(tapleaf.script[33], 0x75); // OP_DROP
    EXPECT_EQ(tapleaf.script[34], 0x20); // OP_PUSH32
    EXPECT_EQ(tapleaf.script[35], 0xBB); // meta start
    EXPECT_EQ(tapleaf.script[66], 0xBB); // meta end
    EXPECT_EQ(tapleaf.script[67], 0x75); // OP_DROP
    EXPECT_EQ(tapleaf.script[68], 0x51); // OP_TRUE

    // Compare with expected leaf hash so it is stable across runs
    std::array<uint8_t, 32> expected_hash = {
        0x68, 0x86, 0x50, 0xa5, 0xe0, 0x9b, 0x0c, 0x18,
        0xde, 0x01, 0x11, 0x29, 0xd0, 0x23, 0xed, 0x63,
        0xa0, 0x19, 0xd8, 0x30, 0x81, 0xe3, 0x78, 0x63,
        0x3e, 0x5b, 0x75, 0x94, 0x81, 0x89, 0x46, 0x5b
    };

    EXPECT_EQ(std::memcmp(tapleaf.leaf_hash.data(), expected_hash.data(), 32), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
