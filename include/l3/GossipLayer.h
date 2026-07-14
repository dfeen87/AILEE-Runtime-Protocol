#pragma once

#include <cstdint>
#include "l2/ExecutionContext.h"
#include "MeshCoherence.h"

namespace ailee {
namespace l3 {

// ---------------------------------------------------------
// Gossip flags (signature + health)
// ---------------------------------------------------------
constexpr uint32_t GOSSIP_FLAG_HEALTHY          = 1 << 0;
constexpr uint32_t GOSSIP_FLAG_STALE            = 1 << 1;
constexpr uint32_t GOSSIP_FLAG_NEEDS_RECOVERY   = 1 << 2;

// NEW — signature validation flags
constexpr uint32_t GOSSIP_FLAG_INVALID_SIGNATURE = 1 << 3;   // NEW
constexpr uint32_t GOSSIP_FLAG_VALID_SIGNATURE   = 1 << 4;   // NEW

// ---------------------------------------------------------
// GossipSummary (unchanged)
// ---------------------------------------------------------
struct alignas(64) GossipSummary {
    uint8_t node_identity_fingerprint[32];
    uint8_t state_root_hash[32];
    uint8_t epoch_hash[32];
    uint64_t epoch_height;
    uint64_t coherence_score;
    uint8_t flags;
    uint8_t padding[15];
};
static_assert(sizeof(GossipSummary) == 128, "GossipSummary must be 128 bytes");

// ---------------------------------------------------------
// GossipMessage (unchanged)
// ---------------------------------------------------------
struct alignas(64) GossipMessage {
    GossipSummary summary;
    uint64_t sequence_number;
    uint8_t message_hash[32];
    uint8_t padding[24];
};
static_assert(sizeof(GossipMessage) == 192, "GossipMessage must be 192 bytes");

// ---------------------------------------------------------
// GossipEnvelope — UPDATED
// ---------------------------------------------------------
struct alignas(64) GossipEnvelope {
    GossipSummary remote_summary;   // 128 bytes
    uint64_t received_sequence;
    uint8_t normalized_hash[32];
    uint8_t padding[24];

    // NEW — signature validation flags
    uint32_t flags = 0;             // NEW
};
static_assert(sizeof(GossipEnvelope) == 192, "GossipEnvelope must be 192 bytes");

// ---------------------------------------------------------
// Pure functions
// ---------------------------------------------------------
GossipMessage build_gossip_message(
    const l2::ExecutionEnvelope& envelope,
    const mesh::MeshCoherenceResult& coherence,
    uint64_t current_sequence
);

GossipEnvelope receive_gossip_message(const GossipMessage& message);

} // namespace l3
} // namespace ailee
