#include "wave_native_routing_layer.hpp"
#include <cmath>

namespace ailee {
namespace wnn {

double RoutingEngine::calculate_hybrid_distance(double latency, double jitter, double phase_mismatch, double freq_mismatch, double stability_divergence) {
    double distance =
        RoutingWeights::W_L * latency +
        RoutingWeights::W_J * jitter +
        RoutingWeights::W_PHI * std::abs(phase_mismatch) +
        RoutingWeights::W_OMEGA * std::abs(freq_mismatch) +
        RoutingWeights::W_DPHI * stability_divergence;

    return distance;
}

} // namespace wnn
} // namespace ailee
