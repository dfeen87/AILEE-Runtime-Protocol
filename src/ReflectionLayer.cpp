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
        if (slice.size() == sizeof(CacheAlignedAnchor)) {
            // Memory layout of CacheAlignedAnchor is fixed (sizeof == 128)
            // but the populated data fields are:
            // 32 bytes anchor_hash (offset 0)
            // 8 bytes block_height (offset 64 due to alignment padding or packed layout)
            // To be perfectly safe against padding differences, we just copy the raw aligned struct bytes.
            std::memcpy(&out_anchor, slice.data(), sizeof(CacheAlignedAnchor));
            return true;
        } else if (slice.size() == 40) {
            // Explicitly support the compact 40 byte packed structure without alignas padding.
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
        if (slice.size() == sizeof(CacheAlignedReorgEvent)) {
            std::memcpy(&out_reorg, slice.data(), sizeof(CacheAlignedReorgEvent));
            return true;
        } else if (slice.size() == 80) {
            // Explicitly support compact 80 byte format: old_height(8) + new_height(8) + old_hash(32) + new_hash(32)
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
