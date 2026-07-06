#pragma once

#include <cstdint>
#include <cstddef>

namespace ailee {

// Forward declarations for dependencies
namespace build {
struct BuildHashInfo;
}
struct GenesisInfo;
struct ConfigInfo;

class RocksDbHandle;
class HeartbeatLog;
class StateRootLog;

namespace mesh {

struct MeshNodeId {
    uint8_t id[32];
};

struct alignas(64) MeshNodeSnapshot {
    MeshNodeId node_id;

    uint64_t latest_l1_height;
    uint8_t  latest_anchor_hash[32];

    uint64_t latest_l2_epoch;
    uint8_t  latest_state_root[32];
};

struct MeshCoherenceResult {
    MeshNodeId self_id;
    MeshNodeId other_id;

    uint32_t score; // 0-4

    bool l1_height_match;
    bool anchor_match;
    bool epoch_match;
    bool state_root_match;
};

struct MeshCoherenceSummary {
    MeshNodeId self_id;

    uint32_t total_nodes;
    uint32_t fully_coherent_nodes;     // score == 4
    uint32_t partially_coherent_nodes; // score 1-3
    uint32_t divergent_nodes;          // score == 0
};

// Builder functions
MeshNodeId compute_node_id(
    const build::BuildHashInfo& build,
    const GenesisInfo& genesis,
    const ConfigInfo& config
);

MeshNodeSnapshot build_local_snapshot(
    const MeshNodeId& self_id,
    const RocksDbHandle& rocksdb,
    const HeartbeatLog& heartbeat,
    const StateRootLog& state_log
);

// Deterministic Coherence Scoring
MeshCoherenceResult compute_mesh_coherence(
    const MeshNodeSnapshot& self,
    const MeshNodeSnapshot& other
);

// Mesh-Wide Coherence Summary
MeshCoherenceSummary summarize_mesh_coherence(
    const MeshNodeSnapshot& self,
    const MeshNodeSnapshot* others,
    size_t count
);

} // namespace mesh
} // namespace ailee
