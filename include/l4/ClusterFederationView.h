#pragma once

#include <vector>
#include <cstddef>
#include "l4/ClusterTypes.h"

namespace ailee {
namespace l4 {

class ClusterSim; // Forward declaration
struct InFlightEnvelope; // Forward declaration

struct ClusterFederationView {
    std::vector<ClusterView> cluster_views;

    struct FederationEnvelopeStats {
        size_t in_flight;
        size_t delivered;
        size_t pending;
    } envelope_stats;

    struct FederationCoherenceSummary {
        double average_coherence;
        double min_coherence;
        double max_coherence;
    } coherence_summary;
};

ClusterFederationView build_federation_view(
    const std::vector<ClusterSim>& clusters,
    const std::vector<InFlightEnvelope>& in_flight
);

} // namespace l4
} // namespace ailee
