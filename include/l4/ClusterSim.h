#pragma once

#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>
#include "l4/ClusterTypes.h"

namespace ailee {
namespace l4 {

class DeterministicScheduler;

class ClusterSim {
public:
    ClusterSim();
    ~ClusterSim();

    // Prevent copying because of the unique ptr to scheduler
    ClusterSim(const ClusterSim&) = delete;
    ClusterSim& operator=(const ClusterSim&) = delete;

    // Allow moving
    ClusterSim(ClusterSim&&) noexcept;
    ClusterSim& operator=(ClusterSim&&) noexcept;

    // Run one full deterministic 9‑phase cycle
    void run_cluster_cycle();

    // Inject an envelope arriving from another cluster
    void inject_envelope(const l2::ExecutionEnvelope& env);

    // Collect envelopes produced during this cycle
    std::vector<l2::ExecutionEnvelope> collect_outgoing_envelopes();

    // Accessor for building ClusterView
    ClusterView build_view() const;

private:
    std::vector<l2::ExecutionEnvelope> inbox;
    std::vector<l2::ExecutionEnvelope> outbox;

    ClusterView view;
    std::vector<l2::DeterministicEngine> engines;
    std::vector<std::pair<size_t, size_t>> gossip_schedule;

    // Forward declare and use pointer to avoid circular dependency
    DeterministicScheduler* scheduler;
};

// Backward-compatible free function wrapper
ClusterView run_cluster_simulation(
    const std::vector<ClusterNodeState>& initial_nodes,
    const std::vector<std::pair<size_t, size_t>>& gossip_schedule,
    uint64_t max_steps
);

ClusterCoherenceSummary compute_cluster_coherence(const ClusterView& view);

} // namespace l4
} // namespace ailee
