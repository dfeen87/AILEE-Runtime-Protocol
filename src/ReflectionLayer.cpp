#include "ReflectionLayer.h"
#include "L1Reflection.h"
#include <cstring>
#include <rocksdb/slice.h>

namespace ailee {
namespace reflection {

bool reflect_latest_block_height(
    const RocksDbHandle& db,
    CacheAlignedBlockHeight& out_height
) {
    rocksdb::Slice slice;
    if (db.get_raw_block_height_slice(slice)) {
        if (slice.size() == sizeof(uint64_t)) {
            std::memcpy(&out_height.height, slice.data(), sizeof(uint64_t));
            return true;
        }
    }
    return false;
}

bool reflect_latest_anchor(
    const RocksDbHandle& db,
    CacheAlignedAnchor& out_anchor
) {
    rocksdb::Slice slice;
    if (db.get_raw_anchor_slice(slice)) {
        if (slice.size() == sizeof(CacheAlignedAnchor) || slice.size() == 32 + sizeof(uint64_t)) {
            // Because struct may have padding due to alignas(64) applied to the hash array,
            // we should be careful with memcpy. The memory layout for CacheAlignedAnchor:
            // - anchor_hash: 32 bytes at offset 0
            // - block_height: 8 bytes at offset 32 (padding? offset might be 32 if alignas(64) is just for the struct alignment)
            // Wait, alignas(64) on uint8_t anchor_hash[32] forces it to be aligned to 64 bytes.
            // But since it's the first member, it aligns the whole struct to 64 bytes.
            // Assuming the slice contains exactly 40 bytes (32 for hash, 8 for height):
            // We just memcpy into the fields.
            const uint8_t* data = reinterpret_cast<const uint8_t*>(slice.data());
            std::memcpy(out_anchor.anchor_hash, data, 32);
            std::memcpy(&out_anchor.block_height, data + 32, sizeof(uint64_t));
            return true;
        }
    }
    return false;
}

bool reflect_reorg_event(
    const RocksDbHandle& db,
    CacheAlignedReorgEvent& out_reorg
) {
    rocksdb::Slice slice;
    if (db.get_raw_reorg_slice(slice)) {
        // Layout: old_height(8) + new_height(8) + old_anchor_hash(32) + new_anchor_hash(32) = 80 bytes
        if (slice.size() == sizeof(uint64_t) * 2 + 32 * 2) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(slice.data());
            std::memcpy(&out_reorg.old_height, data, sizeof(uint64_t));
            std::memcpy(&out_reorg.new_height, data + sizeof(uint64_t), sizeof(uint64_t));
            std::memcpy(out_reorg.old_anchor_hash, data + sizeof(uint64_t) * 2, 32);
            std::memcpy(out_reorg.new_anchor_hash, data + sizeof(uint64_t) * 2 + 32, 32);
            return true;
        }
    }
    return false;
}

ReflectionSnapshot build_reflection_snapshot(
    const RocksDbHandle& db
) {
    ReflectionSnapshot snap;
    std::memset(&snap, 0, sizeof(snap));

    if (!reflect_latest_block_height(db, snap.height)) {
        std::memset(&snap.height, 0, sizeof(snap.height));
    }
    if (!reflect_latest_anchor(db, snap.anchor)) {
        std::memset(&snap.anchor, 0, sizeof(snap.anchor));
    }
    if (!reflect_reorg_event(db, snap.reorg)) {
        std::memset(&snap.reorg, 0, sizeof(snap.reorg));
    }

    return snap;
}

uint64_t reflection_get_height(const ReflectionSnapshot& snap) {
    return snap.height.height;
}

const uint8_t* reflection_get_anchor_hash(const ReflectionSnapshot& snap) {
    return snap.anchor.anchor_hash;
}

uint64_t reflection_get_anchor_height(const ReflectionSnapshot& snap) {
    return snap.anchor.block_height;
}

} // namespace reflection
} // namespace ailee
