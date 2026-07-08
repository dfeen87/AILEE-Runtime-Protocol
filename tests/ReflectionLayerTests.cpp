#include "ReflectionLayer.h"
#include "L1Reflection.h"
#include <gtest/gtest.h>
#include <rocksdb/slice.h>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>

using namespace ailee::reflection;
using namespace ailee;

class MockRocksDbHandle : public RocksDbHandle {
public:
    std::vector<uint8_t> block_height_data;
    std::vector<uint8_t> anchor_data;
    std::vector<uint8_t> reorg_data;

    bool return_success = true;

    uint64_t get_latest_l1_height() const override { return 0; }

    void get_latest_confirmed_anchor(uint8_t /*out_hash*/[32]) const override {
    }

    // FIX: remove parameter names
    bool get_raw_value(const std::string& /*key*/, rocksdb::Slice& /*out_slice*/) const override {
        return false;
    }

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
