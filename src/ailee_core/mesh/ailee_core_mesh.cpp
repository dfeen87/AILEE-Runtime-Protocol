#include "ailee_core_mesh.hpp"

namespace ailee {
namespace core {
namespace mesh {

bool WnnMeshIntegration::filter_transduction_routes(
    ailee::wnn::RoutingEngine& wnnRouter,
    const ailee::ambient_mesh::AmbientMeshRoutingEngine& l2Router,
    const ailee::ambient_mesh::AmbientMeshMessage& message,
    uint32_t currentMessagesPerSecond,
    double latency, double jitter, double phase_mismatch, double freq_mismatch, double stability_divergence
) const {
    double distance = wnnRouter.calculate_hybrid_distance(latency, jitter, phase_mismatch, freq_mismatch, stability_divergence);

    if (distance > 10.0) {
        return false;
    }

    return l2Router.validateRouteConstraints(message, currentMessagesPerSecond);
}

} // namespace mesh
} // namespace core
} // namespace ailee
