#pragma once

#include "semantics/SemanticsTypes.h"

namespace ailee::semantics {

struct MeshCoherenceSemantics {
    static double compute_coherence_score(const MeshState& state);
    static bool is_coherent_enough(double score, const PolicyState& policy);
};

} // namespace ailee::semantics
