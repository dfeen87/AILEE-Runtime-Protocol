#pragma once

#include "ailee/governance/GovernanceConstitution.hpp"
#include <string>
#include <vector>

namespace ailee::approval {

struct ApprovalFactors {
    uint32_t quorum_count{0};
    uint32_t total_validators{0};
    bool operator_signature_valid{false};
    bool system_signature_valid{false};
    bool zk_state_consistent{false};
    double posture_score{0.0};
    double temporal_coherence_index{0.0};
    std::string zk_recursion_root;
};

struct GateDecision {
    bool approved{false};
    bool quorum_passed{false};
    bool signatures_valid{false};
    bool zk_valid{false};
    bool posture_passed{false};
    bool coherence_passed{false};
    std::string rejection_reason;
    std::vector<std::string> evaluated_factors;
};

class ApprovalGate {
public:
    ApprovalGate() : constitution_() {}
    explicit ApprovalGate(governance::GovernanceConstitution constitution)
        : constitution_(constitution) {}

    GateDecision evaluate_factors(const ApprovalFactors& factors) const;
    void set_constitution(const governance::GovernanceConstitution& constitution) { constitution_ = constitution; }

private:
    governance::GovernanceConstitution constitution_;
};

} // namespace ailee::approval
