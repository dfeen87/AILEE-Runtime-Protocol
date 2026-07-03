#include "ailee_api.hpp"
#include <algorithm>

namespace ailee {

Advisory ailee_advise_action(const BitcoinSnapshot& snapshot, const Policy& policy, const AileeContext& context) {
    PostureReport posture = ailee_evaluate_posture(snapshot, context);

    bool approved = true;
    std::string action = "BROADCAST";

    if (posture.risk.value > policy.max_acceptable_risk_score) {
        approved = false;
        action = "HOLD";
    }

    if (snapshot.current_fee_rate > policy.max_fee_rate_sats_per_vbyte) {
        approved = false;
        action = "HOLD";
    }

    if (posture.confidence < policy.min_confidence_required) {
        approved = false;
        action = "HOLD";
    }

    // Check if current regime is in allowed regimes
    if (!policy.allowed_regimes.empty()) {
        auto it = std::find(policy.allowed_regimes.begin(), policy.allowed_regimes.end(), posture.regime);
        if (it == policy.allowed_regimes.end()) {
            approved = false;
            action = "HOLD";
        }
    }

    // Check block interval tolerance
    double deviation = std::abs(static_cast<double>(snapshot.block_interval_avg) - 600.0);
    if (deviation > policy.block_interval_tolerance_sec) {
        approved = false;
        action = "HOLD";
    }

    if (approved && posture.risk.band == RiskBand::Medium) {
        action = "ESCALATE_FEE";
    }

    return {posture, action, approved};
}

} // namespace ailee
