#pragma once

#include <vector>
#include <cstdint>
#include <array>

namespace ailee {
namespace l1_sync {

enum class SyncEventType : uint8_t {
    HeaderApplied = 0,
    ReorgDetected = 1,
    MempoolDeltaApplied = 2
};

struct SyncEvent {
    SyncEventType type;
    uint64_t height;
    std::array<uint8_t, 32> block_hash; // Relevant for HeaderApplied and ReorgDetected
    std::array<uint8_t, 32> txid;       // Relevant for MempoolDeltaApplied
};

using SyncEventBatch = std::vector<SyncEvent>;

} // namespace l1_sync
} // namespace ailee
