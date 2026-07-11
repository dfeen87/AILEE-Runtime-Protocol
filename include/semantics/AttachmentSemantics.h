#pragma once

#include "l6/ProofScheduling.h"
#include "l6/ZKProvingBackend.h"
#include "semantics/SemanticsTypes.h"

namespace ailee::semantics {

struct AttachmentSemantics {
    static bool should_anchor(const l6::SchedulerDecision& decision, const PolicyState& policy, double coherence_score, uint64_t epoch_id);
    static bool should_attach_proof(const l6::SchedulerDecision& decision, const PolicyState& policy, l6::ZKBackendType backend_type, double coherence_score, uint64_t epoch_id);
};

} // namespace ailee::semantics
