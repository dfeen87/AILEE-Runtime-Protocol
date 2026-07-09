#include <gtest/gtest.h>
#include "anchor/AnchorCommitTxBuilder.h"
#include <cstring>

using namespace ailee::anchor;

TEST(AnchorCommitTxBuilderTests, DeterministicSerialization) {
    std::array<uint8_t, 32> anchor_root;
    std::fill(anchor_root.begin(), anchor_root.end(), 0xAA);

    std::array<uint8_t, 32> metadata_hash;
    std::fill(metadata_hash.begin(), metadata_hash.end(), 0xBB);

    AnchorCommitInput input;
    std::fill(input.prev_txid.begin(), input.prev_txid.end(), 0xCC);
    input.prev_vout = 1;
    input.value_sats = 10000;
    std::fill(input.internal_key.begin(), input.internal_key.end(), 0xDD);

    uint64_t fee_sats = 150;

    AnchorCommitTx tx = AnchorCommitTxBuilder::build_anchor_commit_tx(
        anchor_root, metadata_hash, input, fee_sats);

    const std::vector<uint8_t>& raw = tx.raw_tx;

    // Check minimum length (version + marker/flag + 1 in + 1 out + empty witness + locktime)
    EXPECT_TRUE(raw.size() > 60);

    size_t offset = 0;

    // Version = 2
    EXPECT_EQ(raw[offset], 2);
    EXPECT_EQ(raw[offset+1], 0);
    EXPECT_EQ(raw[offset+2], 0);
    EXPECT_EQ(raw[offset+3], 0);
    offset += 4;

    // Marker = 0x00, Flag = 0x01
    EXPECT_EQ(raw[offset], 0x00);
    EXPECT_EQ(raw[offset+1], 0x01);
    offset += 2;

    // 1 input
    EXPECT_EQ(raw[offset], 1);
    offset += 1;

    // prev_txid
    EXPECT_EQ(std::memcmp(&raw[offset], input.prev_txid.data(), 32), 0);
    offset += 32;

    // prev_vout = 1
    EXPECT_EQ(raw[offset], 1);
    EXPECT_EQ(raw[offset+1], 0);
    EXPECT_EQ(raw[offset+2], 0);
    EXPECT_EQ(raw[offset+3], 0);
    offset += 4;

    // scriptSig length = 0
    EXPECT_EQ(raw[offset], 0);
    offset += 1;

    // sequence = 0xFFFFFFFF
    EXPECT_EQ(raw[offset], 0xFF);
    EXPECT_EQ(raw[offset+1], 0xFF);
    EXPECT_EQ(raw[offset+2], 0xFF);
    EXPECT_EQ(raw[offset+3], 0xFF);
    offset += 4;

    // 1 output
    EXPECT_EQ(raw[offset], 1);
    offset += 1;

    // value_sats = 10000 - 150 = 9850
    uint64_t output_value = 9850;
    EXPECT_EQ(std::memcmp(&raw[offset], &output_value, 8), 0);
    offset += 8;

    // scriptPubKey length (P2TR is 34 bytes: OP_1 + OP_PUSH32 + 32 bytes)
    EXPECT_EQ(raw[offset], 34);
    offset += 1;

    // scriptPubKey (starts with OP_1 (0x51), OP_PUSH32 (0x20))
    EXPECT_EQ(raw[offset], 0x51);
    EXPECT_EQ(raw[offset+1], 0x20);
    offset += 34;

    // Witness items (0 items for 1 input)
    EXPECT_EQ(raw[offset], 0);
    offset += 1;

    // locktime = 0
    EXPECT_EQ(raw[offset], 0);
    EXPECT_EQ(raw[offset+1], 0);
    EXPECT_EQ(raw[offset+2], 0);
    EXPECT_EQ(raw[offset+3], 0);
    offset += 4;

    EXPECT_EQ(offset, raw.size());
}
