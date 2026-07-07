#include "l4/MultiClusterSim.h"
#include <algorithm>
#include <cstring>

namespace ailee {
namespace l4 {

MultiClusterSim::MultiClusterSim(const FederationConfig& cfg)
    : config(cfg)
{
    clusters.reserve(cfg.cluster_count);
    for (size_t i = 0; i < cfg.cluster_count; ++i) {
        clusters.emplace_back(ClusterSim());
    }
}

void MultiClusterSim::run_federation_tick() {
    for (auto& cluster : clusters) {
        cluster.run_cluster_cycle();
    }

    propagate_cross_cluster();

    auto view = build_view();

    // In a real implementation, we would extract the full state/telemetry for federation recording.
    // For now, we simulate recording the current federation view.
    // Since ReplayBuffer currently takes deterministic scheduler state, cluster view, and telemetry,
    // we may need a specialized recording for federation if ReplayBuffer isn't updated.
    // But per instructions, we just call federation_replay.record_tick({}, view, {})
    // Note: ReplayBuffer takes ClusterView, not ClusterFederationView. We will just record the first cluster view
    // to satisfy the signature if ReplayBuffer isn't updated, but the spec says `federation_replay.record_tick({}, view, {});`
    // where view is a ClusterFederationView. ReplayBuffer currently expects `const DeterministicSchedulerState&`,
    // `const ClusterView&`, and `const TelemetrySample&`. We'll use dummy parameters for now to compile.

    DeterministicSchedulerState dummy_state = {};
    TelemetrySample dummy_telemetry = {};
    if (!view.cluster_views.empty()) {
        federation_replay.record_tick(dummy_state, view.cluster_views.front(), dummy_telemetry);
    }

    federation_tick++;
}

void MultiClusterSim::propagate_cross_cluster() {
    for (auto it = in_flight.begin(); it != in_flight.end();) {
        if (it->deliver_at_tick == federation_tick) {
            clusters[it->target_cluster].inject_envelope(it->envelope);
            it = in_flight.erase(it);
        } else {
            ++it;
        }
    }

    for (size_t i = 0; i < clusters.size(); ++i) {
        auto outgoing = clusters[i].collect_outgoing_envelopes();

        for (const auto& env : outgoing) {
            for (size_t j = 0; j < clusters.size(); ++j) {
                if (config.routing_matrix[i][j]) {
                    in_flight.push_back({
                        env,
                        federation_tick + config.cross_cluster_latency,
                        j
                    });
                }
            }
        }
    }

    std::sort(in_flight.begin(), in_flight.end(),
        [](const auto& a, const auto& b) {
            // ExecutionEnvelope has context_hash[32] we can compare deterministically
            return std::memcmp(a.envelope.context.context_hash, b.envelope.context.context_hash, 32) < 0;
        });
}

ClusterFederationView MultiClusterSim::build_view() const {
    return build_federation_view(clusters, in_flight);
}

} // namespace l4
} // namespace ailee
