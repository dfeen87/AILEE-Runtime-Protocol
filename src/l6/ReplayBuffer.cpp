#include "l6/ReplayBuffer.h"
#include <rocksdb/db.h>
#include <stdexcept>
#include <jsoncpp/json/json.h>

namespace ailee::l6 {

ReplayBuffer::ReplayBuffer(const ReplayBufferConfig& config) : db_(nullptr) {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, config.db_path, &db_);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB for ReplayBuffer: " + status.ToString());
    }
}

ReplayBuffer::~ReplayBuffer() {
    delete db_;
}

void ReplayBuffer::record_epoch(const EpochIntegrationBundle& bundle, const IslaEpochResult& result) {
    if (!db_) return;

    // Serialize to JSON for simplicity in V27
    Json::Value root;
    root["epoch_id"] = static_cast<Json::UInt64>(bundle.epoch_id);
    root["state_root_hash"] = bundle.state_root_hash;

    // Save minimal clock state
    Json::Value clock_json;
    clock_json["height"] = static_cast<Json::UInt64>(bundle.clock_snapshot.height);
    clock_json["timestamp"] = static_cast<Json::UInt64>(bundle.clock_snapshot.timestamp);
    root["clock_snapshot"] = clock_json;

    // Save minimal decision
    Json::Value sched_json;
    sched_json["anchor_decision"] = static_cast<uint32_t>(bundle.scheduler_decision.anchor_decision);
    sched_json["proof_decision"] = static_cast<uint32_t>(bundle.scheduler_decision.proof_decision);
    root["scheduler_decision"] = sched_json;

    // Save metadata_hash (hex encoded)
    std::string meta_hex = "";
    char hex_chars[] = "0123456789abcdef";
    for (uint8_t b : result.metadata_hash) {
        meta_hex += hex_chars[b >> 4];
        meta_hex += hex_chars[b & 0x0F];
    }
    root["metadata_hash"] = meta_hex;

    Json::FastWriter writer;
    std::string value = writer.write(root);

    std::string key = "epoch_" + std::to_string(bundle.epoch_id);

    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
    if (!status.ok()) {
        throw std::runtime_error("Failed to write to ReplayBuffer: " + status.ToString());
    }
}

} // namespace ailee::l6
