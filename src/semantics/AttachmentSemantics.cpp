#include "semantics/AttachmentSemantics.h"
#include "semantics/MeshCoherenceSemantics.h"

namespace ailee::semantics {

bool AttachmentSemantics::should_anchor(const l6::SchedulerDecision& decision, const PolicyState& policy, double coherence_score, uint64_t epoch_id) {
    if (decision.anchor_decision == l6::AnchorDecision::SKIP) {
        return false;
    }

    if (policy.environment_type != EnvironmentType::CI) {
        if (!MeshCoherenceSemantics::is_coherent_enough(coherence_score, policy)) {
            return false;
        }
    }

    // In V27 semantics, if policy has an explicit anchor frequency, we should respect it
    // unless we are in CI where we trust the mock scheduler for unit test boundaries.
    if (policy.environment_type != EnvironmentType::CI) {
        if (policy.anchor_frequency > 0 && (epoch_id % policy.anchor_frequency) != 0) {
            return false;
        }
    }

    return true;
}

bool AttachmentSemantics::should_attach_proof(const l6::SchedulerDecision& decision, const PolicyState& policy, l6::ZKBackendType backend_type, double coherence_score, uint64_t epoch_id) {
    // Backend type semantics
    if (policy.environment_type == EnvironmentType::CI && backend_type != l6::ZKBackendType::MOCK) {
        return false; // Forbidden in CI unless MOCK
    }
    if (policy.environment_type == EnvironmentType::PROD && backend_type == l6::ZKBackendType::MOCK) {
        return false; // MOCK forbidden in PROD
    }

    if (decision.proof_decision == l6::ProofDecision::SKIP) {
        return false;
    }

    if (policy.environment_type != EnvironmentType::CI) {
        if (!MeshCoherenceSemantics::is_coherent_enough(coherence_score, policy)) {
            return false;
        }
    }

    if (policy.environment_type != EnvironmentType::CI) {
        if (policy.proof_frequency > 0 && (epoch_id % policy.proof_frequency) != 0) {
            return false;
        }
    }

    return true;
}

} // namespace ailee::semantics
