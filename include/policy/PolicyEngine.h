#pragma once

#include "l6/ProofScheduling.h"
#include "l6/ZKProvingBackend.h"
#include <string>

namespace ailee::policy {

struct PolicyConfig {
    uint32_t required_coherence_score;
    uint32_t anchor_frequency;
    bool enforce_strict_proofs;
    std::string allowed_backend_env; // "CI" or "PROD"
};

class PolicyEngine {
public:
    explicit PolicyEngine(const PolicyConfig& config);
    ~PolicyEngine() = default;

    // Based on the given epoch and coherence score, generates a SchedulerDecision
    l6::SchedulerDecision compute_epoch_decision(uint64_t epoch_id, double mesh_coherence_score) const;

    // Validates whether the active backend is permitted by policy
    bool is_backend_allowed(l6::ZKBackendType backend_type, bool is_ci) const;

private:
    PolicyConfig config_;
};

} // namespace ailee::policy
