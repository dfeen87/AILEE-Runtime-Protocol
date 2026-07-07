#pragma once

#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>
#include "l4/ClusterTypes.h"

namespace ailee {
namespace l4 {

ClusterView run_cluster_simulation(
    const std::vector<ClusterNodeState>& initial_nodes,
    const std::vector<std::pair<size_t, size_t>>& gossip_schedule,
    uint64_t max_steps
);

ClusterCoherenceSummary compute_cluster_coherence(const ClusterView& view);

} // namespace l4
} // namespace ailee
