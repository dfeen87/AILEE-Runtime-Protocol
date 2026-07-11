#pragma once

#include "MeshCoherence.h"
#include <vector>
#include <memory>

namespace ailee::l6 {

class MeshCoherenceEngine {
public:
    MeshCoherenceEngine() = default;
    ~MeshCoherenceEngine() = default;

    // Track a peer's reported state root and current clock snapshot.
    // In V27, this determines the "temporal" and "structural" coherence of the peer.
    void register_peer_state(const mesh::MeshNodeSnapshot& peer_snapshot);

    // Get the coherence summary based on the current local snapshot
    mesh::MeshCoherenceSummary get_coherence_summary(const mesh::MeshNodeSnapshot& local_snapshot) const;

    // Returns a coherence score normalized to [0.0, 1.0]
    double get_normalized_coherence_score(const mesh::MeshNodeSnapshot& local_snapshot) const;

private:
    std::vector<mesh::MeshNodeSnapshot> peers_;
};

} // namespace ailee::l6
