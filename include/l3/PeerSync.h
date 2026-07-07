#pragma once

#include <cstdint>
#include "l2/ExecutionContext.h"
#include "l3/GossipLayer.h"

namespace ailee {
namespace l3 {

enum class SyncStatus : uint8_t {
    IN_SYNC = 0,
    AHEAD = 1,
    BEHIND = 2,
    NEEDS_RECOVERY = 3,
    STALE = 4
};

constexpr uint64_t STALE_THRESHOLD = 12;

struct alignas(64) PeerSyncState {
    l2::ExecutionEnvelope local_envelope; // 256 bytes
    GossipEnvelope remote_envelope; // 192 bytes
    int64_t coherence_delta; // 8 bytes
    SyncStatus sync_status; // 1 byte
    uint8_t padding[55]; // 256 + 192 + 8 + 1 + 55 = 512 bytes
};
static_assert(sizeof(PeerSyncState) == 512, "PeerSyncState must be 512 bytes");

struct alignas(64) PeerSyncSummary {
    uint64_t local_epoch; // 8 bytes
    uint64_t remote_epoch; // 8 bytes
    int64_t delta; // 8 bytes
    uint8_t sync_status; // 1 byte
    uint8_t padding[39]; // 8 + 8 + 8 + 1 + 39 = 64 bytes
};
static_assert(sizeof(PeerSyncSummary) == 64, "PeerSyncSummary must be 64 bytes");

// Pure functions
PeerSyncState compute_peer_sync(
    const l2::ExecutionEnvelope& local_envelope,
    const GossipEnvelope& remote_envelope
);

PeerSyncSummary summarize_peer_sync(const PeerSyncState& state);

} // namespace l3
} // namespace ailee
