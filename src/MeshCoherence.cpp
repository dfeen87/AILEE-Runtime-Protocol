#include "MeshCoherence.h"
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

    // Serialize deterministic fields into a fixed-width buffer
    // build_hash (32) || protocol_version_hash (32) || genesis_anchor_root (32) || config_hash (32)
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
    offset += 32;

    compute_sha256(buffer, buffer_size, node_id.id);

    return node_id;
}

MeshNodeSnapshot build_local_snapshot(
    const MeshNodeId& self_id,
    const RocksDbHandle& rocksdb,
    const HeartbeatLog& heartbeat,
    const StateRootLog& state_log
) {
    MeshNodeSnapshot snapshot;
    std::memset(&snapshot, 0, sizeof(snapshot));

    snapshot.node_id = self_id;
    snapshot.latest_l1_height = rocksdb.get_latest_l1_height();
    rocksdb.get_latest_confirmed_anchor(snapshot.latest_anchor_hash);

    snapshot.latest_l2_epoch = heartbeat.get_latest_epoch();
    state_log.get_latest_state_root(snapshot.latest_state_root);

    return snapshot;
}

MeshCoherenceResult compute_mesh_coherence(
    const MeshNodeSnapshot& self,
    const MeshNodeSnapshot& other
) {
    MeshCoherenceResult result;
    std::memset(&result, 0, sizeof(result));

    result.self_id = self.node_id;
    result.other_id = other.node_id;

    if (self.latest_l1_height == other.latest_l1_height) {
        result.score++;
        result.l1_height_match = true;
    }

    if (std::memcmp(self.latest_anchor_hash, other.latest_anchor_hash, 32) == 0) {
        result.score++;
        result.anchor_match = true;
    }

    if (self.latest_l2_epoch == other.latest_l2_epoch) {
        result.score++;
        result.epoch_match = true;
    }

    if (std::memcmp(self.latest_state_root, other.latest_state_root, 32) == 0) {
        result.score++;
        result.state_root_match = true;
    }

    return result;
}

MeshCoherenceSummary summarize_mesh_coherence(
    const MeshNodeSnapshot& self,
    const MeshNodeSnapshot* others,
    size_t count
) {
    MeshCoherenceSummary summary;
    std::memset(&summary, 0, sizeof(summary));

    summary.self_id = self.node_id;
    summary.total_nodes = count;

    for (size_t i = 0; i < count; ++i) {
        MeshCoherenceResult result = compute_mesh_coherence(self, others[i]);
        if (result.score == 4) {
            summary.fully_coherent_nodes++;
        } else if (result.score > 0) {
            summary.partially_coherent_nodes++;
        } else {
            summary.divergent_nodes++;
        }
    }

    return summary;
}

} // namespace mesh
} // namespace ailee
