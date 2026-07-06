#pragma once

#include "ambient_ai_mesh_routing_engine.hpp"
#include "../../wnn/routing/wave_native_routing_layer.hpp"

namespace ailee {
namespace core {
namespace mesh {

class WnnMeshIntegration {
public:
    // RoutingEngine -> AmbientMesh
    bool filter_transduction_routes(
        ailee::wnn::RoutingEngine& wnnRouter,
        const ailee::ambient_mesh::AmbientMeshRoutingEngine& l2Router,
        const ailee::ambient_mesh::AmbientMeshMessage& message,
        uint32_t currentMessagesPerSecond,
        double latency, double jitter, double phase_mismatch, double freq_mismatch, double stability_divergence
    ) const;
};

} // namespace mesh
} // namespace core
} // namespace ailee
