#include "l6/MeshCoherenceEngine.h"
#include <algorithm>
#include <cstring>

namespace ailee::l6 {

void MeshCoherenceEngine::register_peer_state(const mesh::MeshNodeSnapshot& peer_snapshot) {
    // Check if peer already exists, update if so
    auto it = std::find_if(peers_.begin(), peers_.end(), [&peer_snapshot](const mesh::MeshNodeSnapshot& p) {
        return std::memcmp(p.node_id.id, peer_snapshot.node_id.id, 32) == 0;
    });

    if (it != peers_.end()) {
        *it = peer_snapshot;
    } else {
        peers_.push_back(peer_snapshot);
    }
}

mesh::MeshCoherenceSummary MeshCoherenceEngine::get_coherence_summary(const mesh::MeshNodeSnapshot& local_snapshot) const {
    if (peers_.empty()) {
        mesh::MeshCoherenceSummary empty_summary;
        empty_summary.self_id = local_snapshot.node_id;
        empty_summary.total_nodes = 0;
        empty_summary.fully_coherent_nodes = 0;
        empty_summary.partially_coherent_nodes = 0;
        empty_summary.divergent_nodes = 0;
        return empty_summary;
    }

    return mesh::summarize_mesh_coherence(local_snapshot, peers_.data(), peers_.size());
}

double MeshCoherenceEngine::get_normalized_coherence_score(const mesh::MeshNodeSnapshot& local_snapshot) const {
    mesh::MeshCoherenceSummary summary = get_coherence_summary(local_snapshot);
    if (summary.total_nodes == 0) return 1.0; // 100% coherent if alone

    // Weight fully coherent more than partially
    double score =
        (summary.fully_coherent_nodes * 1.0) +
        (summary.partially_coherent_nodes * 0.5) +
        (summary.divergent_nodes * 0.0);

    return score / summary.total_nodes;
}

} // namespace ailee::l6
