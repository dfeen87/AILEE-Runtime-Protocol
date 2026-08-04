// Licensed under the PolyForm Noncommercial License 1.0.0
// PersistentStorage.cpp — RocksDB-based persistent storage implementation

#include "PersistentStorage.h"
#include <stdexcept>
#include <iostream>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>

namespace ailee::storage {

class PersistentStorage::Impl {
public:
    rocksdb::DB* db = nullptr;
    
    ~Impl() {
        if (db) {
            delete db;
            db = nullptr;
        }
    }
};

PersistentStorage::PersistentStorage(const Config& config) 
    : impl_(std::make_unique<Impl>()) {
    rocksdb::Options options;
    
    // Basic options
    options.create_if_missing = config.createIfMissing;
    options.max_open_files = static_cast<int>(config.maxOpenFiles);
    
    // Performance tuning
    options.write_buffer_size = config.writeBufferSizeMB * 1024 * 1024;
    
    // Block cache for reads
    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = rocksdb::NewLRUCache(config.blockCacheSizeMB * 1024 * 1024);
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
    
    // Compression
    options.compression = rocksdb::kSnappyCompression;
    
    // Open database
    rocksdb::Status status = rocksdb::DB::Open(options, config.dbPath, &impl_->db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB at " + config.dbPath + ": " + status.ToString());
    }
    
    std::cout << "[PersistentStorage] Initialized RocksDB at: " << config.dbPath << std::endl;
}

PersistentStorage::~PersistentStorage() = default;

bool PersistentStorage::put(const std::string& key, const std::string& value) {
    if (!impl_->db) {
        return false;
    }
    
    rocksdb::WriteOptions writeOptions;
    writeOptions.sync = true; // Ensure deterministic atomicity
    
    rocksdb::Status status = impl_->db->Put(writeOptions, key, value);
    return status.ok();
}

std::optional<std::string> PersistentStorage::get(const std::string& key) {
    if (!impl_->db) {
        return std::nullopt;
    }
    
    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);
    
    if (status.ok()) {
        return value;
    } else if (status.IsNotFound()) {
        return std::nullopt;
    } else {
        std::cerr << "[PersistentStorage] Get failed for key " << key << ": " << status.ToString() << std::endl;
        return std::nullopt;
    }
}

bool PersistentStorage::remove(const std::string& key) {
    if (!impl_->db) {
        return false;
    }
    
    rocksdb::Status status = impl_->db->Delete([]() { rocksdb::WriteOptions o; o.sync = true; return o; }(), key);
    return status.ok();
}

bool PersistentStorage::exists(const std::string& key) {
    if (!impl_->db) {
        return false;
    }
    
    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);
    return status.ok();
}

class PersistentStorage::WriteBatch::Impl {
public:
    rocksdb::WriteBatch batch;
    size_t count = 0;
};

PersistentStorage::WriteBatch::WriteBatch() : impl_(std::make_unique<Impl>()) {}
PersistentStorage::WriteBatch::~WriteBatch() = default;
PersistentStorage::WriteBatch::WriteBatch(WriteBatch&&) noexcept = default;
PersistentStorage::WriteBatch& PersistentStorage::WriteBatch::operator=(WriteBatch&&) noexcept = default;

void PersistentStorage::WriteBatch::put(const std::string& key, const std::string& value) {
    impl_->batch.Put(key, value);
    impl_->count++;
}

void PersistentStorage::WriteBatch::remove(const std::string& key) {
    impl_->batch.Delete(key);
    impl_->count++;
}

void PersistentStorage::WriteBatch::clear() {
    impl_->batch.Clear();
    impl_->count = 0;
}

bool PersistentStorage::commitBatch(const WriteBatch& batch) {
    if (!batch.getImpl() || batch.getImpl()->count == 0) {
        std::cerr << "[PersistentStorage] commitBatch failed: empty or malformed batch" << std::endl;
        return false;
    }
    if (!impl_->db) {
        std::cerr << "[PersistentStorage] commitBatch failed: database not open" << std::endl;
        return false;
    }

    rocksdb::WriteOptions writeOptions;
    writeOptions.sync = true; // Ensure deterministic atomicity

    // Const-cast away the constness of WriteBatch because RocksDB's Write()
    // signature confusingly takes a non-const WriteBatch pointer (even though
    // it conceptually just reads the batch to write it).
    rocksdb::WriteBatch* mutableBatch = const_cast<rocksdb::WriteBatch*>(&batch.getImpl()->batch);
    rocksdb::Status status = impl_->db->Write(writeOptions, mutableBatch);
    if (!status.ok()) {
        std::cerr << "[PersistentStorage] commitBatch failed: " << status.ToString() << std::endl;
        return false;
    }
    return true;
}

bool PersistentStorage::executeBatch(const std::vector<BatchOp>& ops) {
    if (ops.empty()) {
        std::cerr << "[PersistentStorage] executeBatch failed: empty or malformed batch" << std::endl;
        return false;
    }
    if (!impl_->db) {
        std::cerr << "[PersistentStorage] executeBatch failed: database not open" << std::endl;
        return false;
    }

    rocksdb::WriteBatch batch;
    for (const auto& op : ops) {
        if (op.type == BatchOpType::PUT) {
            batch.Put(op.key, op.value);
        } else if (op.type == BatchOpType::DEL) {
            batch.Delete(op.key);
        }
    }

    rocksdb::WriteOptions writeOptions;
    writeOptions.sync = true; // Ensure data is synced to disk for batch operations

    rocksdb::Status status = impl_->db->Write(writeOptions, &batch);
    if (!status.ok()) {
        std::cerr << "[PersistentStorage] executeBatch failed: " << status.ToString() << std::endl;
        return false;
    }
    return true;
}

} // namespace ailee::storage
