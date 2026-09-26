#include "ailee/posture/PostureEngine.hpp"
#include <algorithm>
#include <cmath>

namespace ailee::posture {

PostureResult PostureEngine::evaluate(const PostureEvaluationInput& input) const {
    // Sanitize and clamp inputs
    double current_fee_rate = std::max(0.0, std::isnan(input.current_fee_rate) ? 0.0 : input.current_fee_rate);
    double high_fee_band = std::max(1.0, std::isnan(input.high_fee_band) ? 50.0 : input.high_fee_band);
    double recent_volatility = std::max(0.0, std::min(1.0, std::isnan(input.recent_volatility) ? 0.0 : input.recent_volatility));
    double signal_coherence = std::max(0.0, std::min(1.0, std::isnan(input.signal_coherence) ? 1.0 : input.signal_coherence));

    double score = 0.0;

    // 1. Fee Rate Risk
    if (current_fee_rate > high_fee_band * 1.5) {
        score += 3.0;
    } else if (current_fee_rate > high_fee_band) {
        score += 1.5;
    }

    // 2. Volatility Risk
    if (recent_volatility > 0.8) {
        score += 3.0;
    } else if (recent_volatility > 0.5) {
        score += 1.5;
    }

    // 3. Block Time / Delay Risk
    double deviation = std::abs(static_cast<double>(input.block_interval_avg) - 600.0);
    if (deviation > 300.0) {
        score += 2.0;
    }
    if (input.time_since_last_block > 1800) {
        score += 2.0;
    }

    // Cap at 10.0
    score = std::min(10.0, std::max(0.0, score));

    // Regime classification
    PostureRegime regime = PostureRegime::NEUTRAL;
    std::string regime_id = "neutral";

    if (recent_volatility > 0.7 && input.daily_change_pct > 5.0) {
        regime = PostureRegime::PARABOLIC;
        regime_id = "parabolic";
    } else if (recent_volatility > 0.7 && input.daily_change_pct < -5.0) {
        regime = PostureRegime::RISK_OFF;
        regime_id = "risk-off";
    } else if (recent_volatility < 0.3) {
        regime = PostureRegime::CHOP;
        regime_id = "chop";
    } else if (score >= 7.0) {
        regime = PostureRegime::STRESS;
        regime_id = "stress";
    } else if (score < 3.0 && signal_coherence > 0.8) {
        regime = PostureRegime::RECOVERY;
        regime_id = "recovery";
    }

    std::string summary;
    if (score < 3.0) {
        summary = "Network conditions stable with high temporal coherence.";
    } else if (score < 6.0) {
        summary = "Moderate volatility or fee pressure. Operational within bounds.";
    } else if (score < 8.5) {
        summary = "Elevated network stress or delay. Caution advised.";
    } else {
        summary = "Extreme network stress. Non-critical operations deferred.";
    }

    double confidence = 1.0;
    if (input.time_since_last_block > 3600) {
        confidence = 0.5;
    }

    return {score, regime, regime_id, summary, confidence, signal_coherence};
}

std::string PostureEngine::regime_to_string(PostureRegime regime) {
    switch (regime) {
        case PostureRegime::NEUTRAL: return "NEUTRAL";
        case PostureRegime::PARABOLIC: return "PARABOLIC";
        case PostureRegime::RISK_OFF: return "RISK_OFF";
        case PostureRegime::CHOP: return "CHOP";
        case PostureRegime::STRESS: return "STRESS";
        case PostureRegime::RECOVERY: return "RECOVERY";
    }
    return "UNKNOWN";
}

} // namespace ailee::posture
