#include <gtest/gtest.h>
#include "l6/ReplayBuffer.h"
#include <filesystem>
#include <rocksdb/db.h>

using namespace ailee::l6;

TEST(ReplayBufferTest, InitializationAndRecord) {
    std::string test_db = "test_replay_db";
    std::filesystem::remove_all(test_db);

    {
        ReplayBufferConfig config{test_db};
        ReplayBuffer buffer(config);

        EpochIntegrationBundle bundle;
        bundle.epoch_id = 42;
        bundle.state_root_hash = "deadbeef";
        bundle.clock_snapshot = {1000, 1600000000, "0000000000000000000000000000000000000000000000000000000000000000", "mock"};
        bundle.scheduler_decision = {AnchorDecision::ANCHOR, ProofDecision::ATTACH_PROOF};

        IslaEpochResult result;
        result.metadata_hash.fill(1);

        buffer.record_epoch(bundle, result);
    }

    // verify
    rocksdb::DB* db = nullptr;
    rocksdb::Options options;
    rocksdb::Status status = rocksdb::DB::Open(options, test_db, &db);
    EXPECT_TRUE(status.ok());
    if (status.ok()) {
        std::string value;
        status = db->Get(rocksdb::ReadOptions(), "epoch_42", &value);
        EXPECT_TRUE(status.ok());
        EXPECT_NE(value.find("deadbeef"), std::string::npos);
        EXPECT_NE(value.find("42"), std::string::npos);
        delete db;
    }

    std::filesystem::remove_all(test_db);
}
