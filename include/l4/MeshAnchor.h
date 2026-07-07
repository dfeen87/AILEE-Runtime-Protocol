#pragma once

#include <cstdint>

namespace ailee {
namespace l4 {

struct ClusterView;

struct alignas(64) MeshEpoch {
    uint64_t epoch_height;         // 8 bytes
    uint8_t epoch_hash[32];        // 32 bytes
    uint8_t mesh_state_root[32];   // 32 bytes
    // Total size: 72 bytes
    // To reach multiple of 64 bytes (128 bytes), need 128 - 72 = 56 bytes padding
    uint8_t padding[56];
};
static_assert(sizeof(MeshEpoch) == 128, "MeshEpoch must be 128 bytes");

struct alignas(64) MeshAnchor {
    uint64_t anchor_id;            // 8 bytes
    MeshEpoch epoch;               // 128 bytes
    uint64_t total_nodes;          // 8 bytes
    uint64_t in_sync_nodes;        // 8 bytes
    uint64_t recovered_nodes;      // 8 bytes
    uint8_t anchor_hash[32];       // 32 bytes
    // The previous fields add up to 8+128+8+8+8+32 = 192 bytes
    // But because of alignas(64) on MeshEpoch, the struct size calculation might add padding before epoch or after it depending on layout.
    // Let's verify alignment: anchor_id (8 bytes), then epoch (alignas(64)). So 56 bytes padding is implicitly added before epoch.
    // So 8 (anchor_id) + 56 (padding) + 128 (epoch) + 8 + 8 + 8 + 32 = 248.
    // Next multiple is 256. Padding needed: 8 bytes.
    uint8_t padding[8];
};
static_assert(sizeof(MeshAnchor) == 256, "MeshAnchor must be 256 bytes");

struct alignas(64) MeshPropagationEnvelope {
    uint64_t source_node_id_hash;  // 8 bytes
    // padding before anchor: 56 bytes
    // anchor: 256 bytes
    // propagation_hash: 32 bytes
    // Total: 8 + 56 + 256 + 32 = 352
    // Next multiple of 64 is 384. Padding needed: 32 bytes.
    MeshAnchor anchor;
    uint8_t propagation_hash[32];
    uint8_t padding[32];
};
static_assert(sizeof(MeshPropagationEnvelope) == 384, "MeshPropagationEnvelope must be 384 bytes");

// Helpers
MeshEpoch build_mesh_epoch(const ClusterView& view);
MeshAnchor build_mesh_anchor(const MeshEpoch& epoch, const ClusterView& view);
MeshPropagationEnvelope build_mesh_propagation_envelope(
    uint64_t source_node_id_hash,
    const MeshAnchor& anchor);

uint64_t compute_mesh_coherence_score(const MeshAnchor& anchor);

} // namespace l4
} // namespace ailee
