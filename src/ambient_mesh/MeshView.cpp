#include "mesh/MeshView.h"
#include "BuildInfo.h"
#include "ConfigInfo.h"
#include "L1Reflection.h"
#include "HeartbeatLog.h"
#include "StateLog.h"

#include <openssl/sha.h>
#include <cstring>

namespace ailee {
namespace mesh {

namespace {

void compute_sha256(const uint8_t* data, size_t len, uint8_t out_hash[32]) {
    SHA256(data, len, out_hash);
}

} // namespace

MeshNodeId compute_node_id(
    const build::BuildHashInfo& build,
    const GenesisInfo& genesis,
    const ConfigInfo& config
) {
    MeshNodeId node_id;
    std::memset(&node_id, 0, sizeof(node_id));

    // Combine inputs: build_hash || protocol_version_hash || genesis_anchor_root || config_hash
    constexpr size_t buffer_size = 32 + 32 + 32 + 32;
    uint8_t buffer[buffer_size];

    size_t offset = 0;
    std::memcpy(buffer + offset, build.build_hash, 32);
    offset += 32;

    std::memcpy(buffer + offset, build.protocol_version_hash, 32);
    offset += 32;

    std::memcpy(buffer + offset, genesis.genesis_anchor_root, 32);
    offset += 32;

    std::memcpy(buffer + offset, config.config_hash, 32);

    compute_sha256(buffer, buffer_size, node_id.id);

    return node_id;
}

MeshView build_local_mesh_view(
    const RocksDbHandle& rocksdb,
    const HeartbeatLog& heartbeat,
    const StateRootLog& stateroot
) {
    MeshView view;
    std::memset(&view, 0, sizeof(view));

    // Note: self_id is not populated here because it requires BuildInfo, GenesisInfo, and ConfigInfo.
    // It should be set by the caller.

    view.latest_l1_height = rocksdb.get_latest_l1_height();
    rocksdb.get_latest_confirmed_anchor(view.latest_confirmed_anchor_hash);

    view.latest_l2_epoch = heartbeat.get_latest_epoch();
    stateroot.get_latest_state_root(view.latest_l2_state_root);

    view.num_snapshots = 0;

    return view;
}

uint32_t compute_mesh_coherence(const MeshView& self, const MeshView& other) {
    uint32_t score = 0;

    if (self.latest_l1_height == other.latest_l1_height) {
        score += 1;
    }

    if (std::memcmp(self.latest_confirmed_anchor_hash, other.latest_confirmed_anchor_hash, 32) == 0) {
        score += 1;
    }

    if (self.latest_l2_epoch == other.latest_l2_epoch) {
        score += 1;
    }

    if (std::memcmp(self.latest_l2_state_root, other.latest_l2_state_root, 32) == 0) {
        score += 1;
    }

    return score;
}

} // namespace mesh
} // namespace ailee
