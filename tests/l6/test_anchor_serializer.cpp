#include <gtest/gtest.h>
#include "l6/AnchorSerializer.h"

using namespace ailee::l6;

TEST(AnchorSerializerTest, SerializedSizeIsExactly49Bytes) {
    AnchorRecord rec;
    rec.version = 1;
    rec.replay_height = 1000;
    rec.coherence_score = 5000;
    rec.state_root.fill(0xAA);
    
    std::vector<uint8_t> bytes = serialize(rec);
    EXPECT_EQ(bytes.size(), 49);
}

TEST(AnchorSerializerTest, SerializationRoundTrip) {
    AnchorRecord original;
    original.version = 42;
    original.replay_height = 0x1122334455667788ULL;
    original.coherence_score = 0x8877665544332211ULL;
    for (int i = 0; i < 32; ++i) {
        original.state_root[i] = static_cast<uint8_t>(i);
    }
    
    std::vector<uint8_t> bytes = serialize(original);
    
    // Check manual Big-Endian output
    // replay_height
    EXPECT_EQ(bytes[1], 0x11);
    EXPECT_EQ(bytes[2], 0x22);
    EXPECT_EQ(bytes[3], 0x33);
    EXPECT_EQ(bytes[4], 0x44);
    EXPECT_EQ(bytes[5], 0x55);
    EXPECT_EQ(bytes[6], 0x66);
    EXPECT_EQ(bytes[7], 0x77);
    EXPECT_EQ(bytes[8], 0x88);

    // coherence_score
    EXPECT_EQ(bytes[9], 0x88);
    EXPECT_EQ(bytes[10], 0x77);
    EXPECT_EQ(bytes[11], 0x66);
    EXPECT_EQ(bytes[12], 0x55);
    EXPECT_EQ(bytes[13], 0x44);
    EXPECT_EQ(bytes[14], 0x33);
    EXPECT_EQ(bytes[15], 0x22);
    EXPECT_EQ(bytes[16], 0x11);

    AnchorRecord decoded;
    bool success = deserialize(bytes, decoded);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.replay_height, original.replay_height);
    EXPECT_EQ(decoded.coherence_score, original.coherence_score);
    EXPECT_EQ(decoded.state_root, original.state_root);
}

TEST(AnchorSerializerTest, DeserializeFailsOnWrongSize) {
    AnchorRecord decoded;
    
    std::vector<uint8_t> too_short(48, 0);
    EXPECT_FALSE(deserialize(too_short, decoded));
    
    std::vector<uint8_t> too_long(50, 0);
    EXPECT_FALSE(deserialize(too_long, decoded));
}

TEST(AnchorSerializerTest, EdgeCaseZerosAndMax) {
    AnchorRecord zeros;
    zeros.version = 0;
    zeros.replay_height = 0;
    zeros.coherence_score = 0;
    zeros.state_root.fill(0);
    
    std::vector<uint8_t> bytes_zeros = serialize(zeros);
    AnchorRecord decoded_zeros;
    EXPECT_TRUE(deserialize(bytes_zeros, decoded_zeros));
    EXPECT_EQ(decoded_zeros.replay_height, 0);
    EXPECT_EQ(decoded_zeros.coherence_score, 0);
    
    AnchorRecord maxes;
    maxes.version = 255;
    maxes.replay_height = 0xFFFFFFFFFFFFFFFFULL;
    maxes.coherence_score = 0xFFFFFFFFFFFFFFFFULL;
    maxes.state_root.fill(0xFF);
    
    std::vector<uint8_t> bytes_maxes = serialize(maxes);
    AnchorRecord decoded_maxes;
    EXPECT_TRUE(deserialize(bytes_maxes, decoded_maxes));
    EXPECT_EQ(decoded_maxes.replay_height, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(decoded_maxes.coherence_score, 0xFFFFFFFFFFFFFFFFULL);
}
