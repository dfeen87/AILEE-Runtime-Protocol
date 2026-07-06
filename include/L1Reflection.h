#pragma once

#include <cstdint>
#include <string>
#include <rocksdb/slice.h>

namespace ailee {

struct GenesisInfo {
    uint8_t genesis_anchor_root[32];
};

class RocksDbHandle {
public:
    virtual ~RocksDbHandle() = default;

    // Existing methods
    virtual uint64_t get_latest_l1_height() const = 0;
    virtual void get_latest_confirmed_anchor(uint8_t out_hash[32]) const = 0;

    // New zero-copy reflection methods
    virtual bool get_raw_value(const std::string& key, rocksdb::Slice& out_slice) const = 0;
    virtual bool get_raw_anchor_slice(rocksdb::Slice& out_slice) const = 0;
    virtual bool get_raw_reorg_slice(rocksdb::Slice& out_slice) const = 0;
    virtual bool get_raw_block_height_slice(rocksdb::Slice& out_slice) const = 0;
};

} // namespace ailee
