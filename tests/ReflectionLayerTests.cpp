#include <gtest/gtest.h>
#include "ReflectionLayer.h"
#include "L1Reflection.h"
#include <cstring>
#include <vector>
#include <rocksdb/slice.h>

namespace ailee {
namespace reflection {
namespace {

class MockRocksDbHandle : public RocksDbHandle {
public:
    std::vector<uint8_t> block_height_data;
    std::vector<uint8_t> anchor_data;
    std::vector<uint8_t> reorg_data;

    bool return_success = true;

    uint64_t get_latest_l1_height() const override { return 0; }
    void get_latest_confirmed_anchor(uint8_t out_hash[32]) const override {}

    bool get_raw_value(const std::string& key, rocksdb::Slice& out_slice) const override { return false; }

    bool get_raw_anchor_slice(rocksdb::Slice& out_slice) const override {
        if (!return_success || anchor_data.empty()) return false;
        out_slice = rocksdb::Slice(reinterpret_cast<const char*>(anchor_data.data()), anchor_data.size());
        return true;
    }

    bool get_raw_reorg_slice(rocksdb::Slice& out_slice) const override {
        if (!return_success || reorg_data.empty()) return false;
        out_slice = rocksdb::Slice(reinterpret_cast<const char*>(reorg_data.data()), reorg_data.size());
        return true;
    }

    bool get_raw_block_height_slice(rocksdb::Slice& out_slice) const override {
        if (!return_success || block_height_data.empty()) return false;
        out_slice = rocksdb::Slice(reinterpret_cast<const char*>(block_height_data.data()), block_height_data.size());
        return true;
    }
};

TEST(ReflectionLayerTests, CacheAlignment) {
    static_assert(alignof(CacheAlignedBlockHeight) == 64, "CacheAlignedBlockHeight must be 64-byte aligned");
    static_assert(alignof(CacheAlignedAnchor) == 64, "CacheAlignedAnchor must be 64-byte aligned");
    static_assert(alignof(CacheAlignedReorgEvent) == 64, "CacheAlignedReorgEvent must be 64-byte aligned");
    static_assert(alignof(ReflectionSnapshot) == 64, "ReflectionSnapshot must be 64-byte aligned");
}

TEST(ReflectionLayerTests, ZeroCopyCorrectness) {
    MockRocksDbHandle db;

    uint64_t expected_height = 42000;
    db.block_height_data.resize(sizeof(uint64_t));
    std::memcpy(db.block_height_data.data(), &expected_height, sizeof(uint64_t));

    uint8_t expected_hash[32];
    std::memset(expected_hash, 0xAB, 32);
    db.anchor_data.resize(40);
    std::memcpy(db.anchor_data.data(), expected_hash, 32);
    std::memcpy(db.anchor_data.data() + 32, &expected_height, sizeof(uint64_t));

    CacheAlignedBlockHeight height_struct;
    EXPECT_TRUE(reflect_latest_block_height(db, height_struct));
    EXPECT_EQ(height_struct.height, expected_height);

    CacheAlignedAnchor anchor_struct;
    EXPECT_TRUE(reflect_latest_anchor(db, anchor_struct));
    EXPECT_EQ(anchor_struct.block_height, expected_height);
    EXPECT_EQ(std::memcmp(anchor_struct.anchor_hash, expected_hash, 32), 0);
}

TEST(ReflectionLayerTests, DeterministicBehavior) {
    MockRocksDbHandle db;

    uint64_t expected_height = 42000;
    db.block_height_data.resize(sizeof(uint64_t));
    std::memcpy(db.block_height_data.data(), &expected_height, sizeof(uint64_t));

    uint8_t expected_hash[32];
    std::memset(expected_hash, 0xAB, 32);
    db.anchor_data.resize(40);
    std::memcpy(db.anchor_data.data(), expected_hash, 32);
    std::memcpy(db.anchor_data.data() + 32, &expected_height, sizeof(uint64_t));

    uint64_t old_h = 41990;
    uint64_t new_h = 41995;
    uint8_t old_hash[32];
    uint8_t new_hash[32];
    std::memset(old_hash, 0x11, 32);
    std::memset(new_hash, 0x22, 32);
    db.reorg_data.resize(80);
    std::memcpy(db.reorg_data.data(), &old_h, 8);
    std::memcpy(db.reorg_data.data() + 8, &new_h, 8);
    std::memcpy(db.reorg_data.data() + 16, old_hash, 32);
    std::memcpy(db.reorg_data.data() + 48, new_hash, 32);

    ReflectionSnapshot snap1 = build_reflection_snapshot(db);
    ReflectionSnapshot snap2 = build_reflection_snapshot(db);

    EXPECT_EQ(std::memcmp(&snap1, &snap2, sizeof(ReflectionSnapshot)), 0);

    EXPECT_EQ(reflection_get_height(snap1), expected_height);
    EXPECT_EQ(reflection_get_anchor_height(snap1), expected_height);
    EXPECT_EQ(std::memcmp(reflection_get_anchor_hash(snap1), expected_hash, 32), 0);

    EXPECT_EQ(snap1.reorg.old_height, old_h);
    EXPECT_EQ(snap1.reorg.new_height, new_h);
    EXPECT_EQ(std::memcmp(snap1.reorg.old_anchor_hash, old_hash, 32), 0);
    EXPECT_EQ(std::memcmp(snap1.reorg.new_anchor_hash, new_hash, 32), 0);
}

TEST(ReflectionLayerTests, FailureHandling) {
    MockRocksDbHandle db;
    db.return_success = false; // Simulate failure

    ReflectionSnapshot snap = build_reflection_snapshot(db);

    // Everything should be zeroed
    EXPECT_EQ(snap.height.height, 0);
    EXPECT_EQ(snap.anchor.block_height, 0);
    uint8_t zero_hash[32] = {0};
    EXPECT_EQ(std::memcmp(snap.anchor.anchor_hash, zero_hash, 32), 0);
    EXPECT_EQ(snap.reorg.old_height, 0);
    EXPECT_EQ(snap.reorg.new_height, 0);
    EXPECT_EQ(std::memcmp(snap.reorg.old_anchor_hash, zero_hash, 32), 0);
    EXPECT_EQ(std::memcmp(snap.reorg.new_anchor_hash, zero_hash, 32), 0);
}

} // namespace
} // namespace reflection
} // namespace ailee
