#include "wave_native_routing_layer.hpp"

namespace ailee {
namespace wnn {

double RoutingEngine::calculate_hybrid_distance(double /*latency*/, double /*jitter*/, double /*phase_mismatch*/, double /*freq_mismatch*/, double /*stability_divergence*/) {
    return 0.0;
}

} // namespace wnn
} // namespace ailee
