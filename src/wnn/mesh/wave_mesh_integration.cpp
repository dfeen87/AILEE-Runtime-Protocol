#include "wave_mesh_integration.hpp"
#include <vector>

namespace ailee {
namespace wnn {

void MeshOrchestrator::evaluate_topology(const std::vector<MeshNodeState>& nodes) {
    int total_nodes = nodes.size();
    int healthy_nodes = 0;

    for (const auto& node : nodes) {
        bool is_healthy = true;

        if (node.trust_score <= min_trust_threshold_) {
            is_healthy = false;
        }

        if (node.phase_error_deg >= max_phase_error_deg_) {
            is_healthy = false;
        }

        if (node.anchor_id.has_value()) {
            if (!node.pll_locked) {
                is_healthy = false;
            }
            if (!node.coherence_stable) {
                is_healthy = false;
            }
        }

        if (is_healthy) {
            healthy_nodes++;
        }
    }

    double healthy_ratio = 0.0;
    if (total_nodes > 0) {
        healthy_ratio = static_cast<double>(healthy_nodes) / static_cast<double>(total_nodes);
    }

    if (healthy_ratio < DEFAULT_MESH_MIN_HEALTHY_NODE_RATIO) {
        mesh_healthy_ = false;
    } else {
        mesh_healthy_ = true;
    }
}

} // namespace wnn
} // namespace ailee
