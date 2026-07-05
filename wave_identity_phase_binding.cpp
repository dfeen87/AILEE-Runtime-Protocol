#include "wave_identity_phase_binding.hpp"
#include <cmath>

namespace ailee {
namespace wnn {

void CoherenceEngine::evaluate_clusters(const std::vector<CoherenceCluster>& clusters, std::map<space::AnchorId, KnownPeer>& anchors) {
    for (const auto& cluster : clusters) {
        if (cluster.anchors.empty()) continue;

        // Calculate average phase in the cluster (Orthogonal Handshakes emulation)
        long double avg_phase = 0.0;
        int count = 0;
        for (auto anchor_id : cluster.anchors) {
            auto it = anchors.find(anchor_id);
            if (it != anchors.end()) {
                avg_phase += it->second.phase_theta;
                count++;
            }
        }

        if (count > 0) {
            avg_phase /= count;
        }

        // Evaluate each anchor's phase drift
        for (auto anchor_id : cluster.anchors) {
            auto it = anchors.find(anchor_id);
            if (it != anchors.end()) {
                long double drift = std::abs(it->second.phase_theta - avg_phase);
                // If an anchor falls out of phase with its cluster, its trust score collapses
                if (drift > 0.1) {
                    it->second.trust_score = 0.0;
                }
            }
        }
    }
}

} // namespace wnn
} // namespace ailee
