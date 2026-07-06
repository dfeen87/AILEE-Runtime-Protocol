#pragma once

#include <cstdint>
#include <cstddef>

namespace ailee {

struct ConfigInfo;
struct GenesisInfo;
class RocksDbHandle;
class HeartbeatLog;
class StateRootLog;

namespace build {
struct BuildHashInfo;
}

namespace mesh {

struct alignas(64) MeshNodeId {
    uint8_t id[32];
};

struct alignas(64) MeshNodeSnapshot {
    MeshNodeId node_id;
    uint64_t latest_l1_height;
    uint64_t latest_l2_epoch;
    uint8_t latest_l2_state_root[32];
};

struct alignas(64) MeshView {
    MeshNodeId self_id;
    uint64_t latest_l1_height;
    uint8_t latest_confirmed_anchor_hash[32];
    uint64_t latest_l2_epoch;
    uint8_t latest_l2_state_root[32];

    static constexpr size_t MAX_SNAPSHOTS = 16;
    MeshNodeSnapshot snapshots[MAX_SNAPSHOTS];
    size_t num_snapshots;
};

// Pure deterministic builders
MeshNodeId compute_node_id(
    const build::BuildHashInfo& build,
    const GenesisInfo& genesis,
    const ConfigInfo& config
);

MeshView build_local_mesh_view(
    const RocksDbHandle& rocksdb,
    const HeartbeatLog& heartbeat,
    const StateRootLog& stateroot
);

// Pure deterministic scoring
uint32_t compute_mesh_coherence(const MeshView& self, const MeshView& other);

} // namespace mesh
} // namespace ailee
