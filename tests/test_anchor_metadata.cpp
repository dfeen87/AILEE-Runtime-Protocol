#include <gtest/gtest.h>
#include "src/anchor/AnchorMetadata.h"
#include "src/anchor/AnchorMetadataEncoder.h"
#include <openssl/sha.h>
#include <cstring>
#include <vector>
#include <array>

using namespace ailee::anchor;

TEST(AnchorMetadataTest, LittleEndianSerialization) {
    AnchorMetadata meta(
        0x1122334455667788ULL, // epoch_number
        0x99AABBCC,            // version
        0xDDEEFF00,            // replay_window
        0x12345678,            // anchor_type
        0x9ABCDEF0             // flags
    );

    std::vector<uint8_t> bytes = meta.to_bytes();

    // 8 (epoch) + 4 (version) + 4 (replay) + 4 (type) + 4 (flags) = 24 bytes
    ASSERT_EQ(bytes.size(), 24);

    // Verify epoch_number (little-endian)
    EXPECT_EQ(bytes[0], 0x88);
    EXPECT_EQ(bytes[1], 0x77);
    EXPECT_EQ(bytes[2], 0x66);
    EXPECT_EQ(bytes[3], 0x55);
    EXPECT_EQ(bytes[4], 0x44);
    EXPECT_EQ(bytes[5], 0x33);
    EXPECT_EQ(bytes[6], 0x22);
    EXPECT_EQ(bytes[7], 0x11);

    // Verify version (little-endian)
    EXPECT_EQ(bytes[8], 0xCC);
    EXPECT_EQ(bytes[9], 0xBB);
    EXPECT_EQ(bytes[10], 0xAA);
    EXPECT_EQ(bytes[11], 0x99);

    // Verify replay_window (little-endian)
    EXPECT_EQ(bytes[12], 0x00);
    EXPECT_EQ(bytes[13], 0xFF);
    EXPECT_EQ(bytes[14], 0xEE);
    EXPECT_EQ(bytes[15], 0xDD);

    // Verify anchor_type (little-endian)
    EXPECT_EQ(bytes[16], 0x78);
    EXPECT_EQ(bytes[17], 0x56);
    EXPECT_EQ(bytes[18], 0x34);
    EXPECT_EQ(bytes[19], 0x12);

    // Verify flags (little-endian)
    EXPECT_EQ(bytes[20], 0xF0);
    EXPECT_EQ(bytes[21], 0xDE);
    EXPECT_EQ(bytes[22], 0xBC);
    EXPECT_EQ(bytes[23], 0x9A);
}

TEST(AnchorMetadataTest, SHA256Hashing) {
    AnchorMetadata meta(
        100, // epoch_number
        20,  // version
        5,   // replay_window
        1,   // anchor_type
        0    // flags
    );

    std::array<uint8_t, 32> hash = AnchorMetadataEncoder::hash_metadata(meta);

    // Calculate expected hash directly to verify encoder correctness
    std::vector<uint8_t> expected_bytes = meta.to_bytes();
    std::array<uint8_t, 32> expected_hash;
    SHA256(expected_bytes.data(), expected_bytes.size(), expected_hash.data());

    EXPECT_EQ(std::memcmp(hash.data(), expected_hash.data(), 32), 0);
}

TEST(AnchorMetadataTest, KnownHashDeterminism) {
    // A specific test with hardcoded expected hash to ensure cross-platform determinism
    AnchorMetadata meta(
        42, // epoch_number
        20, // version
        10, // replay_window
        2,  // anchor_type
        1   // flags
    );

    // Hardcoded expected SHA-256 hash
    std::array<uint8_t, 32> expected_hash = {
        0x01, 0x45, 0xe3, 0x21, 0xe5, 0xd0, 0xd6, 0xd4,
        0x93, 0x4a, 0x98, 0xf8, 0x4c, 0xdd, 0x6f, 0xf1,
        0x9c, 0x45, 0x61, 0x94, 0x67, 0x99, 0xd1, 0x0f,
        0x92, 0x0d, 0x47, 0xc8, 0xfb, 0x0d, 0x99, 0xd8
    };

    std::array<uint8_t, 32> actual_hash = AnchorMetadataEncoder::hash_metadata(meta);
    EXPECT_EQ(std::memcmp(actual_hash.data(), expected_hash.data(), 32), 0);
}
