#include "l4/MeshAnchor.h"
#include "l4/ClusterSim.h"
#include "l4/ClusterTypes.h"
#include <openssl/sha.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace ailee {
namespace l4 {

namespace {

// Helper to serialize uint64_t into a byte array in little-endian format
void serialize_uint64_le(uint64_t val, uint8_t out[8]) {
    out[0] = static_cast<uint8_t>(val & 0xFF);
    out[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    out[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    out[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    out[4] = static_cast<uint8_t>((val >> 32) & 0xFF);
    out[5] = static_cast<uint8_t>((val >> 40) & 0xFF);
    out[6] = static_cast<uint8_t>((val >> 48) & 0xFF);
    out[7] = static_cast<uint8_t>((val >> 56) & 0xFF);
}

} // anonymous namespace

MeshEpoch build_mesh_epoch(const ClusterView& view) {
    MeshEpoch epoch = {};
    std::memset(&epoch, 0, sizeof(epoch));

    if (view.nodes.empty()) {
        return epoch; // Return zeroed epoch if no nodes
    }

    // Determine epoch_height from the first node (assuming cluster is generally on same epoch for anchoring)
    epoch.epoch_height = view.nodes[0].last_envelope.context.l1_height;

    // Sort nodes by node_id_hash
    std::vector<const ClusterNodeState*> sorted_nodes;
    sorted_nodes.reserve(view.nodes.size());
    for (const auto& node : view.nodes) {
        sorted_nodes.push_back(&node);
    }
    std::sort(sorted_nodes.begin(), sorted_nodes.end(),
              [](const ClusterNodeState* a, const ClusterNodeState* b) {
                  return a->node_id_hash < b->node_id_hash;
              });

    // Hash epoch_height + concatenated state_roots for epoch_hash
    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        uint8_t height_bytes[8];
        serialize_uint64_le(epoch.epoch_height, height_bytes);
        SHA256_Update(&ctx, height_bytes, 8);

        for (const auto* node : sorted_nodes) {
            SHA256_Update(&ctx, node->last_envelope.context.state_root_hash, 32);
        }

        SHA256_Final(epoch.epoch_hash, &ctx);
    }

    // Hash concatenated state_roots for mesh_state_root
    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        for (const auto* node : sorted_nodes) {
            SHA256_Update(&ctx, node->last_envelope.context.state_root_hash, 32);
        }

        SHA256_Final(epoch.mesh_state_root, &ctx);
    }

    return epoch;
}

MeshAnchor build_mesh_anchor(const MeshEpoch& epoch, const ClusterView& view) {
    MeshAnchor anchor = {};
    std::memset(&anchor, 0, sizeof(anchor));

    anchor.epoch = epoch;
    anchor.total_nodes = view.total_nodes;

    ClusterCoherenceSummary coherence = compute_cluster_coherence(view);
    anchor.in_sync_nodes = coherence.in_sync_count;
    anchor.recovered_nodes = coherence.recovered_nodes_count;

    // anchor_id: deterministic hash of epoch + cluster logic (here we use epoch_hash)
    // To simplify and keep deterministic, we derive anchor_id from the first 8 bytes of epoch_hash
    uint64_t anchor_id = 0;
    for (int i = 0; i < 8; ++i) {
        anchor_id |= (static_cast<uint64_t>(epoch.epoch_hash[i]) << (i * 8));
    }
    anchor.anchor_id = anchor_id;

    // anchor_hash: hash of epoch_hash + mesh_state_root + total_nodes + in_sync_nodes + recovered_nodes
    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        SHA256_Update(&ctx, epoch.epoch_hash, 32);
        SHA256_Update(&ctx, epoch.mesh_state_root, 32);

        uint8_t tmp[8];
        serialize_uint64_le(anchor.total_nodes, tmp);
        SHA256_Update(&ctx, tmp, 8);

        serialize_uint64_le(anchor.in_sync_nodes, tmp);
        SHA256_Update(&ctx, tmp, 8);

        serialize_uint64_le(anchor.recovered_nodes, tmp);
        SHA256_Update(&ctx, tmp, 8);

        SHA256_Final(anchor.anchor_hash, &ctx);
    }

    return anchor;
}

MeshPropagationEnvelope build_mesh_propagation_envelope(
    uint64_t source_node_id_hash,
    const MeshAnchor& anchor) {

    MeshPropagationEnvelope env = {};
    std::memset(&env, 0, sizeof(env));

    env.source_node_id_hash = source_node_id_hash;
    env.anchor = anchor;

    // propagation_hash: hash of source_node_id_hash + anchor_hash
    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        uint8_t tmp[8];
        serialize_uint64_le(source_node_id_hash, tmp);
        SHA256_Update(&ctx, tmp, 8);

        SHA256_Update(&ctx, anchor.anchor_hash, 32);

        SHA256_Final(env.propagation_hash, &ctx);
    }

    return env;
}

uint64_t compute_mesh_coherence_score(const MeshAnchor& anchor) {
    if (anchor.total_nodes == 0) {
        return 0;
    }
    return (anchor.in_sync_nodes * 100) / anchor.total_nodes;
}

} // namespace l4
} // namespace ailee
