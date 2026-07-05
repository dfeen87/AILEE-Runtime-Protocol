// SPDX-License-Identifier: MIT  
// PersistentStorage.h — Production persistence layer using RocksDB
// Stores nodes, tasks, proofs, and telemetry with ACID guarantees

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace ailee::storage {

/**
 * Persistent storage layer for AILEE-Core components.
 * 
 * Storage Schema:
 * - nodes/{pubkey} → NodeState (serialized)
 * - tasks/{task_id} → TaskRecord
 * - proofs/{proof_hash} → ZKProof
 * - telemetry/{node_id}/{timestamp} → TelemetrySample
 */
class PersistentStorage {
public:
    struct Config {
        std::string dbPath;
        size_t maxOpenFiles;
        size_t writeBufferSizeMB;
        size_t blockCacheSizeMB;
        bool createIfMissing;
        
        Config()
            : dbPath("./data/ailee.db")
            , maxOpenFiles(1000)
            , writeBufferSizeMB(64)
            , blockCacheSizeMB(512)
            , createIfMissing(true)
        {}
    };
    
    explicit PersistentStorage(const Config& config = Config());
    ~PersistentStorage();
    
    // Disable copy, allow move
    PersistentStorage(const PersistentStorage&) = delete;
    PersistentStorage& operator=(const PersistentStorage&) = delete;
    PersistentStorage(PersistentStorage&&) = default;
    PersistentStorage& operator=(PersistentStorage&&) = default;
    
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool remove(const std::string& key);
    bool exists(const std::string& key);

    enum class BatchOpType { PUT, DEL };
    struct BatchOp {
        BatchOpType type;
        std::string key;
        std::string value; // Only used for PUT
    };
    bool executeBatch(const std::vector<BatchOp>& ops);

    class WriteBatch {
    public:
        WriteBatch();
        ~WriteBatch();

        // Disable copy, allow move
        WriteBatch(const WriteBatch&) = delete;
        WriteBatch& operator=(const WriteBatch&) = delete;
        WriteBatch(WriteBatch&&) noexcept;
        WriteBatch& operator=(WriteBatch&&) noexcept;

        void put(const std::string& key, const std::string& value);
        void remove(const std::string& key);
        void clear();

        class Impl;
        const Impl* getImpl() const { return impl_.get(); }

    private:
        std::unique_ptr<Impl> impl_;
    };

    bool commitBatch(const WriteBatch& batch);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ailee::storage
