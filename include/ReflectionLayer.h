#pragma once

#include <cstdint>

namespace ailee {

class RocksDbHandle;
struct GenesisInfo;

namespace reflection {

struct CacheAlignedBlockHeight {
    alignas(64) uint64_t height;
};

struct CacheAlignedAnchor {
    alignas(64) uint8_t anchor_hash[32];
    uint64_t block_height;
};

struct CacheAlignedReorgEvent {
    alignas(64) uint64_t old_height;
    uint64_t new_height;
    uint8_t old_anchor_hash[32];
    uint8_t new_anchor_hash[32];
};

struct ReflectionSnapshot {
    alignas(64) CacheAlignedBlockHeight height;
    alignas(64) CacheAlignedAnchor anchor;
    alignas(64) CacheAlignedReorgEvent reorg;
};

bool reflect_latest_block_height(
    const RocksDbHandle& db,
    CacheAlignedBlockHeight& out_height
);

bool reflect_latest_anchor(
    const RocksDbHandle& db,
    CacheAlignedAnchor& out_anchor
);

bool reflect_reorg_event(
    const RocksDbHandle& db,
    CacheAlignedReorgEvent& out_reorg
);

ReflectionSnapshot build_reflection_snapshot(
    const RocksDbHandle& db
);

uint64_t reflection_get_height(const ReflectionSnapshot& snap);
const uint8_t* reflection_get_anchor_hash(const ReflectionSnapshot& snap);
uint64_t reflection_get_anchor_height(const ReflectionSnapshot& snap);

} // namespace reflection
} // namespace ailee
