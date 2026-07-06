#pragma once

#include <cstdint>

namespace ailee {

struct GenesisInfo {
    uint8_t genesis_anchor_root[32];
};

class RocksDbHandle {
public:
    virtual ~RocksDbHandle() = default;
    virtual uint64_t get_latest_l1_height() const = 0;
    virtual void get_latest_confirmed_anchor(uint8_t out_hash[32]) const = 0;
};

} // namespace ailee
