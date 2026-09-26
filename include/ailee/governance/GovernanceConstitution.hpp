#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ailee::governance {

struct ConstitutionRules {
    uint32_t min_quorum_threshold{3};
    double min_posture_score{2.5};
    double min_temporal_coherence{0.70};
    bool require_operator_signature{true};
    bool require_system_signature{true};
    std::string constitution_id{"v37.0.0-canonical-constitution"};
};

class GovernanceConstitution {
public:
    GovernanceConstitution() : rules_() {}
    explicit GovernanceConstitution(ConstitutionRules rules) : rules_(rules) {}

    const ConstitutionRules& rules() const { return rules_; }
    void update_rules(const ConstitutionRules& new_rules) { rules_ = new_rules; }

    bool validate_rules() const {
        return rules_.min_quorum_threshold > 0 &&
               rules_.min_posture_score >= 0.0 &&
               rules_.min_temporal_coherence >= 0.0 &&
               rules_.min_temporal_coherence <= 1.0;
    }

private:
    ConstitutionRules rules_;
};

} // namespace ailee::governance
