#include "ailee_api.hpp"
#include <algorithm>
#include <cmath>

namespace ailee {

RiskScore ailee_score_risk(const BitcoinSnapshot& snapshot, const AileeContext& context) {
    (void)context; // Unused for now

    double score = 0.0;

    // 1. Fee Rate Risk
    if (snapshot.current_fee_rate > snapshot.mempool_stats.high_fee_band * 1.5) {
        score += 3.0; // High fees = higher risk of getting stuck or overpaying
    } else if (snapshot.current_fee_rate > snapshot.mempool_stats.high_fee_band) {
        score += 1.5;
    }

    // 2. Volatility Risk
    if (snapshot.recent_volatility > 0.8) {
        score += 3.0;
    } else if (snapshot.recent_volatility > 0.5) {
        score += 1.5;
    }

    // 3. Block Time Risk (Chain stall or rapid blocks)
    // Assuming 600s is target block time
    double deviation = std::abs(static_cast<double>(snapshot.block_interval_avg) - 600.0);
    if (deviation > 300.0) { // Off by more than 5 mins average
        score += 2.0;
    }

    if (snapshot.time_since_last_block > 1800) { // >30 mins since last block
        score += 2.0;
    }

    // Cap at 10.0
    score = std::min(10.0, score);

    RiskBand band;
    if (score < 3.0) band = RiskBand::Low;
    else if (score < 6.0) band = RiskBand::Medium;
    else if (score < 8.5) band = RiskBand::High;
    else band = RiskBand::Extreme;

    return {score, band};
}

PostureReport ailee_evaluate_posture(const BitcoinSnapshot& snapshot, const AileeContext& context) {
    RiskScore risk = ailee_score_risk(snapshot, context);

    std::string regime = snapshot.dominance_or_regime_tag;
    if (regime.empty()) {
        if (snapshot.recent_volatility > 0.7 && snapshot.price_context.daily_change_pct > 5.0) {
            regime = "parabolic";
        } else if (snapshot.recent_volatility > 0.7 && snapshot.price_context.daily_change_pct < -5.0) {
            regime = "risk-off";
        } else if (snapshot.recent_volatility < 0.3) {
            regime = "chop";
        } else {
            regime = "neutral";
        }
    }

    std::string summary;
    if (risk.band == RiskBand::Low) summary = "Network conditions are stable. Good time to transact.";
    else if (risk.band == RiskBand::Medium) summary = "Normal network activity. Proceed with standard fee estimation.";
    else if (risk.band == RiskBand::High) summary = "Elevated risk due to volatility or high fees. Use caution.";
    else summary = "Extreme network conditions. Hold non-urgent transactions.";

    // Simple confidence metric based on whether we have recent blocks
    double confidence = 1.0;
    if (snapshot.time_since_last_block > 3600) {
        confidence = 0.5; // Stale data
    }

    return {risk, regime, summary, confidence};
}

} // namespace ailee
