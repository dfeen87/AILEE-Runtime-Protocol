#include "l4/ClusterFederationView.h"
#include "l4/ClusterSim.h"
#include "l4/MultiClusterSim.h"
#include <algorithm>

namespace ailee {
namespace l4 {

ClusterFederationView build_federation_view(
    const std::vector<ClusterSim>& clusters,
    const std::vector<InFlightEnvelope>& in_flight
) {
    ClusterFederationView view;

    view.cluster_views.reserve(clusters.size());
    double total_coherence = 0.0;
    double min_coherence = 100.0;
    double max_coherence = 0.0;

    for (const auto& cluster : clusters) {
        ClusterView cv = cluster.build_view();
        view.cluster_views.push_back(cv);

        double score = static_cast<double>(cv.coherence_summary.global_coherence_score);
        total_coherence += score;
        min_coherence = std::min(min_coherence, score);
        max_coherence = std::max(max_coherence, score);
    }

    if (!clusters.empty()) {
        view.coherence_summary.average_coherence = total_coherence / clusters.size();
        view.coherence_summary.min_coherence = min_coherence;
        view.coherence_summary.max_coherence = max_coherence;
    } else {
        view.coherence_summary.average_coherence = 0.0;
        view.coherence_summary.min_coherence = 0.0;
        view.coherence_summary.max_coherence = 0.0;
    }

    view.envelope_stats.in_flight = in_flight.size();
    view.envelope_stats.delivered = 0; // Requires tracking over time
    view.envelope_stats.pending = in_flight.size();

    return view;
}

} // namespace l4
} // namespace ailee
