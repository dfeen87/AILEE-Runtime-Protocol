#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

namespace ailee::governance {

struct ConstitutionRules {
    uint32_t min_quorum_threshold{3};
    double max_allowable_posture_score{2.5}; // Upper bound on acceptable posture risk score
    double min_posture_score{2.5};           // Kept for backward compatibility
    double min_temporal_coherence{0.70};
    bool require_operator_signature{true};
    bool require_system_signature{true};
    std::string constitution_id{"v38.0.0-canonical-constitution"};
};

class GovernanceConstitution {
public:
    GovernanceConstitution() : rules_() {}
    explicit GovernanceConstitution(ConstitutionRules rules) : rules_(rules) {
        // Synchronize legacy min_posture_score with max_allowable_posture_score if updated
        if (rules_.max_allowable_posture_score != rules_.min_posture_score) {
            rules_.min_posture_score = rules_.max_allowable_posture_score;
        }
    }

    const ConstitutionRules& rules() const { return rules_; }
    void update_rules(const ConstitutionRules& new_rules) {
        rules_ = new_rules;
        rules_.min_posture_score = rules_.max_allowable_posture_score;
    }

    bool validate_rules() const {
        return rules_.min_quorum_threshold > 0 &&
               std::isfinite(rules_.max_allowable_posture_score) &&
               rules_.max_allowable_posture_score >= 0.0 &&
               rules_.max_allowable_posture_score <= 10.0 &&
               std::isfinite(rules_.min_temporal_coherence) &&
               rules_.min_temporal_coherence >= 0.0 &&
               rules_.min_temporal_coherence <= 1.0;
    }

private:
    ConstitutionRules rules_;
};

} // namespace ailee::governance
