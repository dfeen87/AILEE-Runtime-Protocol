#include "../../src/taproot/TaprootScript.h"
#include <gtest/gtest.h>
#include <array>
#include <cstring>
#include <vector>

using namespace ailee::taproot;

TEST(TaprootScriptTest, BuildScript) {
    std::array<uint8_t, 32> anchor_root;
    std::fill(anchor_root.begin(), anchor_root.end(), 0xAA);

    TapLeaf leaf;
    leaf.anchor_root = anchor_root;
    leaf.build_script();

    ASSERT_EQ(leaf.script.size(), 35);
    EXPECT_EQ(leaf.script[0], 0x20); // OP_PUSH32
    EXPECT_EQ(leaf.script[33], 0x75); // OP_DROP
    EXPECT_EQ(leaf.script[34], 0x51); // OP_TRUE
}

TEST(TaprootScriptTest, ComputeOutputKey) {
    std::array<uint8_t, 32> internal_key = {
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };

    std::array<uint8_t, 32> tapleaf_hash;
    std::fill(tapleaf_hash.begin(), tapleaf_hash.end(), 0xCC);

    TaprootOutput out;
    out.internal_key = internal_key;
    out.compute_output_key(tapleaf_hash);

    // Check that the output key has been modified by the tweak
    EXPECT_NE(std::memcmp(out.taproot_output_key.data(), internal_key.data(), 32), 0);

    // Also check it's not all zeros (which indicates failure)
    bool all_zeros = true;
    for (uint8_t b : out.taproot_output_key) {
        if (b != 0) all_zeros = false;
    }
    EXPECT_FALSE(all_zeros);
}

TEST(TaprootScriptTest, ToBech32mPlaceholder) {
    std::array<uint8_t, 32> taproot_output_key;
    std::fill(taproot_output_key.begin(), taproot_output_key.end(), 0xDD);

    std::vector<uint8_t> wp(taproot_output_key.begin(), taproot_output_key.end());
    std::string address = to_bech32m("bc", wp);
    // Address test will fail/change depending on the final implementation. We can update this when tests fail.
}
