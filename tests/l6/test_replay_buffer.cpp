#include <gtest/gtest.h>
#include "l6/ReplayBuffer.h"
#include "l6/ExternalSchema.h"
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

class ReplayIntrospectionHardener {
public:
    static bool verify_strict_ordering(const std::vector<ExternalReplayTick>& ticks, std::string& error) {
        uint64_t last_id = 0;
        bool first = true;
        for (const auto& tick : ticks) {
            for (const auto& env : tick.cluster_view.envelopes) {
                if (first) {
                    last_id = env.id;
                    first = false;
                } else {
                    if (env.id != last_id + 1) {
                        if (env.id == last_id) {
                            error = "Duplicate envelope detected: ID " + std::to_string(env.id);
                            return false;
                        } else if (env.id < last_id) {
                            error = "Out-of-order envelope detected: ID " + std::to_string(env.id) + " after " + std::to_string(last_id);
                            return false;
                        } else {
                            error = "Missing envelope detected: gap between " + std::to_string(last_id) + " and " + std::to_string(env.id);
                            return false;
                        }
                    }
                    last_id = env.id;
                }
            }
        }
        return true;
    }

    static bool process_idempotent(std::unordered_map<uint64_t, std::string>& processed_state, const ExternalReplayTick& tick) {
        for (const auto& env : tick.cluster_view.envelopes) {
            auto it = processed_state.find(env.id);
            if (it != processed_state.end()) {
                if (it->second != env.payload_hash) {
                    return false; // Conflicting payload for same ID - not idempotent!
                }
            } else {
                processed_state[env.id] = env.payload_hash;
            }
        }
        return true;
    }
};

TEST(ReplayBufferTest, StrictEnvelopeOrderingAndValidation) {
    std::vector<ExternalReplayTick> ticks;

    // Normal sequentially ordered ticks
    {
        ExternalReplayTick tick1;
        tick1.tick_index = 1;
        tick1.cluster_view.envelopes = {{1, 1000, "hash1", "gossip", "delivered"}, {2, 1001, "hash2", "gossip", "delivered"}};
        ticks.push_back(tick1);

        ExternalReplayTick tick2;
        tick2.tick_index = 2;
        tick2.cluster_view.envelopes = {{3, 1002, "hash3", "gossip", "delivered"}};
        ticks.push_back(tick2);
    }

    std::string error;
    EXPECT_TRUE(ReplayIntrospectionHardener::verify_strict_ordering(ticks, error));

    // Duplicate detection
    {
        std::vector<ExternalReplayTick> duplicate_ticks = ticks;
        ExternalReplayTick tick3;
        tick3.tick_index = 3;
        tick3.cluster_view.envelopes = {{3, 1002, "hash3", "gossip", "delivered"}}; // Duplicate of ID 3
        duplicate_ticks.push_back(tick3);

        EXPECT_FALSE(ReplayIntrospectionHardener::verify_strict_ordering(duplicate_ticks, error));
        EXPECT_NE(error.find("Duplicate"), std::string::npos);
    }

    // Out of order detection
    {
        std::vector<ExternalReplayTick> ooo_ticks = ticks;
        ExternalReplayTick tick3;
        tick3.tick_index = 3;
        tick3.cluster_view.envelopes = {{1, 1000, "hash1", "gossip", "delivered"}}; // Out of order ID 1
        ooo_ticks.push_back(tick3);

        EXPECT_FALSE(ReplayIntrospectionHardener::verify_strict_ordering(ooo_ticks, error));
        EXPECT_NE(error.find("Out-of-order"), std::string::npos);
    }

    // Missing gap detection
    {
        std::vector<ExternalReplayTick> missing_ticks = ticks;
        ExternalReplayTick tick3;
        tick3.tick_index = 3;
        tick3.cluster_view.envelopes = {{5, 1005, "hash5", "gossip", "delivered"}}; // Missing ID 4
        missing_ticks.push_back(tick3);

        EXPECT_FALSE(ReplayIntrospectionHardener::verify_strict_ordering(missing_ticks, error));
        EXPECT_NE(error.find("Missing"), std::string::npos);
    }
}

TEST(ReplayBufferTest, ReplayIdempotencyVerification) {
    std::unordered_map<uint64_t, std::string> processed_state;

    ExternalReplayTick tick;
    tick.tick_index = 1;
    tick.cluster_view.envelopes = {{1, 1000, "payload_a", "gossip", "delivered"}};

    // First process
    EXPECT_TRUE(ReplayIntrospectionHardener::process_idempotent(processed_state, tick));
    EXPECT_EQ(processed_state[1], "payload_a");

    // Second processing of exact same tick should be successful & idempotent
    EXPECT_TRUE(ReplayIntrospectionHardener::process_idempotent(processed_state, tick));
    EXPECT_EQ(processed_state[1], "payload_a");

    // Processing same ID with different payload should fail the idempotency guarantee
    ExternalReplayTick conflict_tick;
    conflict_tick.tick_index = 2;
    conflict_tick.cluster_view.envelopes = {{1, 1000, "payload_conflict", "gossip", "delivered"}};

    EXPECT_FALSE(ReplayIntrospectionHardener::process_idempotent(processed_state, conflict_tick));
}
