#include "semantics/MeshCoherenceSemantics.h"

namespace ailee::semantics {

double MeshCoherenceSemantics::compute_coherence_score(const MeshState& state) {
    if (state.node_count == 0) return 0.0;

    double link_density = static_cast<double>(state.active_links) / state.node_count;
    double partition_ratio = static_cast<double>(state.partition_count) / state.node_count;

    double w1 = 0.5;
    double w2 = 0.3;
    double w3 = 0.2;

    double score = w1 * link_density + w2 * (1.0 - state.latency_variance) + w3 * (1.0 - partition_ratio);

    if (score < 0.0) score = 0.0;
    if (score > 1.0) score = 1.0;

    return score;
}

bool MeshCoherenceSemantics::is_coherent_enough(double score, const PolicyState& policy) {
    return score >= policy.min_coherence_score;
}

} // namespace ailee::semantics
