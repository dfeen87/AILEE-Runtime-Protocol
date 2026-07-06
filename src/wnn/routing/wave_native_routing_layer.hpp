#pragma once

#include "../propagation/wave_phase_propagation.hpp"
#include <vector>
#include <optional>
#include <cstdint>

namespace ailee {
namespace wnn {

enum class RoutingMode {
    LOW_LATENCY,
    HIGH_STABILITY,
    BALANCED
};

struct RoutingWeights {
    static constexpr double W_L = 0.15;
    static constexpr double W_J = 0.15;
    static constexpr double W_PHI = 0.30;
    static constexpr double W_OMEGA = 0.20;
    static constexpr double W_DPHI = 0.20;
};

struct RouteDecision {
    TransportVector selected_vector;
    std::optional<std::vector<uint8_t>> destination_peer;
    RoutingMode mode_used;
    double estimated_latency_ms;
    double estimated_stability_score;
    bool is_valid;
};

class RoutingEngine {
public:
    double calculate_hybrid_distance(double latency, double jitter, double phase_mismatch, double freq_mismatch, double stability_divergence);
};

} // namespace wnn
} // namespace ailee
