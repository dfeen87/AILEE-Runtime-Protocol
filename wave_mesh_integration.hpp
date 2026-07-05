#pragma once

#include "wave_identity_phase_binding.hpp"
#include <optional>

namespace ailee {
namespace wnn {

constexpr double DEFAULT_MESH_MIN_HEALTHY_NODE_RATIO = 0.60;

struct MeshNodeState {
    PeerId peer_id;
    std::optional<wnn::space::AnchorId> anchor_id;
    double mesh_density_local;
    double trust_score;
    double phase_error_deg;
    bool pll_locked;
    bool coherence_stable;
};

class MeshOrchestrator {
public:
    void evaluate_topology();
private:
    bool mesh_healthy_;
};

} // namespace wnn
} // namespace ailee
