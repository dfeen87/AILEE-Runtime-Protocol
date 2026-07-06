#include <gtest/gtest.h>
#include "SettlementIngestion.h"
#include <rocksdb/db.h>
#include <filesystem>
#include <memory>

using namespace ailee::l1;

std::string getTestDbPath() {
    static int counter = 0;
    return std::filesystem::temp_directory_path().string() + "/ailee_test_settlement_db_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(counter++);
}

void cleanupTestDb(const std::string& path) {
    rocksdb::DestroyDB(path, rocksdb::Options());
}

TEST(SettlementIngestion, CacheAlignment) {
    EXPECT_EQ(sizeof(CacheAlignedAnchor), 128);
    EXPECT_EQ(alignof(CacheAlignedAnchor), 64);

    EXPECT_EQ(sizeof(CacheAlignedReorgEvent), 128);
    EXPECT_EQ(alignof(CacheAlignedReorgEvent), 64);
}

TEST(SettlementIngestion, HexConversions) {
    CacheAlignedAnchor anchor;
    std::memset(&anchor, 0, sizeof(CacheAlignedAnchor));

    std::string testHash = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
    anchor.setAnchorHashStr(testHash);

    EXPECT_EQ(anchor.getAnchorHashStr(), testHash);
}

TEST(SettlementIngestion, ZeroCopyIntegration) {
    std::string dbPath = getTestDbPath();
    rocksdb::Options options;
    options.create_if_missing = true;

    rocksdb::DB* rawDb = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, dbPath, &rawDb);
    ASSERT_TRUE(status.ok());

    std::shared_ptr<rocksdb::DB> db(rawDb);
    SettlementIngestionEngine engine(db);

    CacheAlignedAnchor anchor;
    std::memset(&anchor, 0, sizeof(CacheAlignedAnchor));
    anchor.setAnchorHashStr("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    anchor.bitcoinHeight = 100;
    anchor.status = static_cast<uint8_t>(AnchorStatus::PENDING);

    std::string err;
    ASSERT_TRUE(engine.writeAnchorZeroCopy(anchor, &err));

    auto readAnchorOpt = engine.getAnchorZeroCopy("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    ASSERT_TRUE(readAnchorOpt.has_value());
    EXPECT_EQ(readAnchorOpt->bitcoinHeight, 100);
    EXPECT_EQ(readAnchorOpt->status, static_cast<uint8_t>(AnchorStatus::PENDING));

    cleanupTestDb(dbPath);
}

TEST(SettlementIngestion, IngestionLogic) {
    std::string dbPath = getTestDbPath();
    rocksdb::Options options;
    options.create_if_missing = true;

    rocksdb::DB* rawDb = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, dbPath, &rawDb);
    ASSERT_TRUE(status.ok());

    std::shared_ptr<rocksdb::DB> db(rawDb);
    SettlementIngestionEngine engine(db);

    CacheAlignedAnchor anchor1;
    std::memset(&anchor1, 0, sizeof(CacheAlignedAnchor));
    anchor1.setAnchorHashStr("1111111111111111111111111111111111111111111111111111111111111111");
    anchor1.bitcoinHeight = 100;
    anchor1.status = static_cast<uint8_t>(AnchorStatus::PENDING);
    std::string err1;
    ASSERT_TRUE(engine.writeAnchorZeroCopy(anchor1, &err1));

    CacheAlignedAnchor anchor2;
    std::memset(&anchor2, 0, sizeof(CacheAlignedAnchor));
    anchor2.setAnchorHashStr("2222222222222222222222222222222222222222222222222222222222222222");
    anchor2.bitcoinHeight = 105;
    anchor2.status = static_cast<uint8_t>(AnchorStatus::PENDING);
    std::string err2;
    ASSERT_TRUE(engine.writeAnchorZeroCopy(anchor2, &err2));

    // Test confirmed ingestion (Height 106, 6 confirmations required -> only 100 should be returned)
    // 106 - 100 + 1 = 7 confirmations >= 6 -> Confirmed
    // 106 - 105 + 1 = 2 confirmations < 6 -> Pending
    auto confirmed = engine.ingestConfirmedAnchors(106, 6);
    ASSERT_EQ(confirmed.size(), 1);
    EXPECT_EQ(confirmed[0].getAnchorHashStr(), "1111111111111111111111111111111111111111111111111111111111111111");

    // Test reorg invalidations (Reorg at height 102)
    auto invalidated = engine.ingestReorgInvalidations(102);
    ASSERT_EQ(invalidated.size(), 1); // Only anchor2 (height 105) is affected
    EXPECT_EQ(invalidated[0].getAnchorHashStr(), "2222222222222222222222222222222222222222222222222222222222222222");

    cleanupTestDb(dbPath);
}
