#include "policy/PolicyEngine.h"

namespace ailee::policy {

PolicyEngine::PolicyEngine(const PolicyConfig& config) : config_(config) {}

l6::SchedulerDecision PolicyEngine::compute_epoch_decision(uint64_t epoch_id, double mesh_coherence_score) const {
    l6::SchedulerDecision decision;

    // Minimum coherence requirement to participate actively (anchor or attach proofs)
    // Scale coherence score to [0, 4] equivalent for the check, or just compare percentages
    // Let's assume required_coherence_score is a percentage 0-100
    double score_percent = mesh_coherence_score * 100.0;
    bool meets_coherence = (score_percent >= config_.required_coherence_score);

    // Anchor decision based on frequency
    if (meets_coherence && config_.anchor_frequency > 0 && (epoch_id % config_.anchor_frequency == 0)) {
        decision.anchor_decision = l6::AnchorDecision::ANCHOR;
    } else {
        decision.anchor_decision = l6::AnchorDecision::SKIP;
    }

    // Proof decision based on strictness and coherence
    if (meets_coherence) {
        decision.proof_decision = l6::ProofDecision::ATTACH_PROOF;
    } else {
        if (config_.enforce_strict_proofs) {
            decision.proof_decision = l6::ProofDecision::SKIP;
        } else {
            // Best effort proofs even if coherence is low but not strict
            decision.proof_decision = l6::ProofDecision::ATTACH_PROOF;
        }
    }

    return decision;
}

bool PolicyEngine::is_backend_allowed(l6::ZKBackendType backend_type, bool is_ci) const {
    if (is_ci) {
        return backend_type == l6::ZKBackendType::MOCK;
    } else {
        if (config_.allowed_backend_env == "CI") return backend_type == l6::ZKBackendType::MOCK;
        return backend_type == l6::ZKBackendType::HALO2_NATIVE || backend_type == l6::ZKBackendType::PLONK_NATIVE;
    }
}

} // namespace ailee::policy
