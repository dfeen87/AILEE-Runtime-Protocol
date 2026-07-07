#pragma once

#include <vector>
#include <cstddef>
#include "l2/DeterministicEngine.h" // Needed for Envelope alias
#include "l4/FederationConfig.h"
#include "l4/ClusterSim.h"
#include "l4/ClusterFederationView.h"
#include "l4/ReplayBuffer.h"

namespace ailee {
namespace l4 {

using Envelope = ailee::l2::ExecutionEnvelope;

struct InFlightEnvelope {
    Envelope envelope;
    size_t deliver_at_tick;
    size_t target_cluster;
};

struct MultiClusterSim {
    FederationConfig config;
    std::vector<ClusterSim> clusters;

    size_t federation_tick = 0;

    std::vector<InFlightEnvelope> in_flight;

    ReplayBuffer federation_replay;

    MultiClusterSim(const FederationConfig& cfg);

    void run_federation_tick();
    void propagate_cross_cluster();
    ClusterFederationView build_view() const;
};

} // namespace l4
} // namespace ailee