#include "ailee_api.hpp"
#include <iostream>

using namespace ailee;

void print_advisory(const Advisory& adv) {
    std::cout << "--- AILEE Advisory Report ---" << std::endl;
    std::cout << "Risk Score: " << adv.posture.risk.value << " / 10.0" << std::endl;
    std::cout << "Risk Band: ";
    switch (adv.posture.risk.band) {
        case RiskBand::Low: std::cout << "Low\n"; break;
        case RiskBand::Medium: std::cout << "Medium\n"; break;
        case RiskBand::High: std::cout << "High\n"; break;
        case RiskBand::Extreme: std::cout << "Extreme\n"; break;
    }
    std::cout << "Regime: " << adv.posture.regime << std::endl;
    std::cout << "Summary: " << adv.posture.summary << std::endl;
    std::cout << "Confidence: " << adv.posture.confidence << std::endl;
    std::cout << "Recommended Action: " << adv.recommended_action << std::endl;
    std::cout << "Action Approved: " << (adv.action_approved ? "YES" : "NO") << std::endl;
    std::cout << "-----------------------------\n" << std::endl;
}

int main() {
    std::cout << "Initializing AILEE Wallet Integration Example...\n";

    AileeConfig config;
    config.environment = "simulation";
    config.strict_mode = true;

    AileeContext ctx = ailee_init(config);

    // Simulate a standard policy
    Policy my_policy;
    my_policy.max_acceptable_risk_score = 6.0;
    my_policy.max_fee_rate_sats_per_vbyte = 50.0;
    my_policy.min_confidence_required = 0.8;
    my_policy.allowed_regimes = {"neutral", "risk-on"};
    my_policy.block_interval_tolerance_sec = 300.0;

    // Scenario 1: Normal conditions
    std::cout << "\n[Scenario 1: Normal Conditions]\n";
    BitcoinSnapshot snap1;
    snap1.height = 800000;
    snap1.price_context = {65000.0, 1.2, 0.2}; // Low volatility
    snap1.mempool_stats = {1000000, 5000, 15.0, 30.0, 5.0};
    snap1.current_fee_rate = 20.0;
    snap1.hashrate_estimate = 550.0;
    snap1.recent_volatility = 0.2;
    snap1.dominance_or_regime_tag = "neutral";
    snap1.time_since_last_block = 300; // 5 mins
    snap1.block_interval_avg = 600;

    Advisory adv1 = ailee_advise_action(snap1, my_policy, ctx);
    print_advisory(adv1);

    // Scenario 2: High fee spike
    std::cout << "\n[Scenario 2: High Fee Spike]\n";
    BitcoinSnapshot snap2 = snap1;
    snap2.current_fee_rate = 80.0; // Higher than policy max
    snap2.recent_volatility = 0.6;
    snap2.dominance_or_regime_tag = "parabolic"; // Not in allowed regimes

    Advisory adv2 = ailee_advise_action(snap2, my_policy, ctx);
    print_advisory(adv2);

    ailee_shutdown(ctx);
    return 0;
}
