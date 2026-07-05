// SPDX-License-Identifier: MIT
// AmbientAI-Core.h — Legacy compatibility header for AmbientAI.
// This header now forwards to AmbientAI.h to avoid duplicate type definitions.

#pragma once

#include "AmbientAI.h"

namespace ambient {

// ==================== SYSTEM & CLUSTER HELPERS ====================

struct ClusterMetrics {
    uint64_t avgLatencyFp = 0;
    uint64_t totalBandwidthFp = 0;
    uint64_t totalComputePowerFp = 0;
    uint64_t energyEfficiencyFp = 0;
    uint64_t geographicDispersionFp = 0;
};

inline ClusterMetrics evaluateCluster(const std::vector<TelemetrySample>& cluster) {
    ClusterMetrics metrics = {};
    if (cluster.empty()) return metrics;

    for (const auto& node : cluster) {
        metrics.avgLatencyFp += node.compute.latencyMsFp;
        metrics.totalBandwidthFp += node.compute.bandwidthMbpsFp;
        metrics.totalComputePowerFp += node.compute.instantaneousPower_GFLOPSFp;
        metrics.energyEfficiencyFp += node.energy.computeEfficiency_GFLOPS_WFp;
    }

    metrics.avgLatencyFp /= cluster.size();
    metrics.energyEfficiencyFp /= cluster.size();

    // sqrt using fixed point scaling isn't standard, but since it's just dispersion of node count:
    // sqrt(size) * SCALE
    uint64_t sizeFp = cluster.size() * FIXED_POINT_SCALE;

    // Simple integer square root implementation
    uint64_t x0 = sizeFp / 2;
    if (x0 != 0) {
        uint64_t x1 = (x0 + sizeFp / x0) / 2;
        while (x1 < x0) {
            x0 = x1;
            x1 = (x0 + sizeFp / x0) / 2;
        }
        metrics.geographicDispersionFp = x0 * 100; // scaling back to match sqrt behavior
    } else {
        metrics.geographicDispersionFp = FIXED_POINT_SCALE;
    }

    return metrics;
}

inline int64_t clusterObjectiveFunctionFp(const ClusterMetrics& m,
                                          uint64_t w_latencyFp = 3000,
                                          uint64_t w_bandwidthFp = 2000,
                                          uint64_t w_computeFp = 3000,
                                          uint64_t w_energyFp = 2000) {
    int64_t latencyTermFp = (w_latencyFp * m.avgLatencyFp) / FIXED_POINT_SCALE;

    uint64_t denomBandwidthFp = m.totalBandwidthFp;
    if (denomBandwidthFp < FIXED_POINT_SCALE) denomBandwidthFp = FIXED_POINT_SCALE; // std::max(1.0, bandwidth)
    int64_t bandwidthTermFp = (w_bandwidthFp * FIXED_POINT_SCALE) / denomBandwidthFp;

    int64_t computeTermFp = -1 * static_cast<int64_t>((w_computeFp * m.totalComputePowerFp) / FIXED_POINT_SCALE);

    uint64_t denomEnergyFp = m.energyEfficiencyFp;
    if (denomEnergyFp < (FIXED_POINT_SCALE / 100)) denomEnergyFp = (FIXED_POINT_SCALE / 100); // std::max(0.01, efficiency)
    int64_t energyTermFp = (w_energyFp * FIXED_POINT_SCALE) / denomEnergyFp;

    return latencyTermFp + bandwidthTermFp + computeTermFp + energyTermFp;
}

} // namespace ambient
