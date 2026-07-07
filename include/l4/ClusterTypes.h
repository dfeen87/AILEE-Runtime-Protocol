#pragma once

#include <vector>
#include <cstdint>
#include "l2/DeterministicEngine.h"
#include "l3/GossipLayer.h"
#include "l3/PeerSync.h"
#include "l4/MeshAnchor.h"

namespace ailee {
namespace l4 {

struct alignas(64) ClusterNodeState {
    l2::EngineState engine_state;                  // 640 bytes
    l2::ExecutionEnvelope last_envelope;           // 256 bytes
    l3::GossipSummary last_gossip_summary;         // 128 bytes
    std::vector<l3::PeerSyncState> peer_sync_states; // 24 bytes (vector)
    uint64_t node_id_hash;                         // 8 bytes
    uint64_t step_counter;                         // 8 bytes
    // Total size: 640 + 256 + 128 + 24 + 8 + 8 = 1064 bytes
    // Next multiple of 64 is 1088. Padding needed: 24 bytes
    uint8_t padding[24];
};
static_assert(sizeof(ClusterNodeState) == 1088, "ClusterNodeState must be 1088 bytes");

struct alignas(64) ClusterView {
    std::vector<ClusterNodeState> nodes; // 24 bytes
    std::vector<MeshPropagationEnvelope> mesh_envelopes; // 24 bytes
    uint64_t total_nodes;                // 8 bytes
    uint64_t total_steps;                // 8 bytes
    // Total size: 24 + 24 + 8 + 8 = 64 bytes
    // Already a multiple of 64. No padding needed!
};
static_assert(sizeof(ClusterView) == 64, "ClusterView must be 64 bytes");

struct alignas(64) ClusterCoherenceSummary {
    uint64_t in_sync_count;             // 8 bytes
    uint64_t ahead_count;               // 8 bytes
    uint64_t behind_count;              // 8 bytes
    uint64_t needs_recovery_count;      // 8 bytes
    uint64_t stale_count;               // 8 bytes
    uint64_t global_coherence_score;    // 8 bytes
    uint64_t recovered_nodes_count;     // 8 bytes
    uint64_t unrecoverable_nodes_count; // 8 bytes
    // Total size: 64 bytes
};
static_assert(sizeof(ClusterCoherenceSummary) == 64, "ClusterCoherenceSummary must be 64 bytes");

} // namespace l4
} // namespace ailee
