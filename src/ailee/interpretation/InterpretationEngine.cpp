#include "ailee/interpretation/InterpretationEngine.hpp"

namespace ailee::interpretation {

InterpretationResult InterpretationEngine::interpret(const InterpretationInput& input) const {
    InterpretationResult result;

    if (input.block_lag_seconds > 3600 || input.coherence_score < 0.3) {
        result.regime = InterpretationRegime::CRITICAL_ISOLATION;
        result.regime_name = "CRITICAL_ISOLATION";
        result.recommendation = "Isolate non-essential sub-systems and await temporal re-alignment.";
        result.triggered_rules.push_back("RULE_CRITICAL_LAG_OR_DECOHERENCE");
        result.confidence = 0.95;
    } else if (input.block_lag_seconds > 1800) {
        result.regime = InterpretationRegime::TEMPORAL_DRIFT;
        result.regime_name = "TEMPORAL_DRIFT";
        result.recommendation = "Re-synchronize with Bitcoin L1 anchor nodes.";
        result.triggered_rules.push_back("RULE_TEMPORAL_DRIFT_EXCEEDED");
        result.confidence = 0.90;
    } else if (input.fee_rate > input.high_fee_band && input.high_fee_band > 0.0) {
        result.regime = InterpretationRegime::MEMPOOL_CONGESTION;
        result.regime_name = "MEMPOOL_CONGESTION";
        result.recommendation = "Adjust transaction batching and optimize fee strategies.";
        result.triggered_rules.push_back("RULE_MEMPOOL_CONGESTION");
        result.confidence = 0.88;
    } else if (input.volatility > 0.6) {
        result.regime = InterpretationRegime::ELEVATED_VOLATILITY;
        result.regime_name = "ELEVATED_VOLATILITY";
        result.recommendation = "Monitor posture metrics closely; maintain conservative posture.";
        result.triggered_rules.push_back("RULE_VOLATILITY_THRESHOLD");
        result.confidence = 0.85;
    } else {
        result.regime = InterpretationRegime::NOMINAL;
        result.regime_name = "NOMINAL";
        result.recommendation = "System operating within nominal deterministic bounds.";
        result.triggered_rules.push_back("RULE_NOMINAL_EXECUTION");
        result.confidence = 0.99;
    }

    return result;
}

std::string InterpretationEngine::regime_to_string(InterpretationRegime regime) {
    switch (regime) {
        case InterpretationRegime::NOMINAL: return "NOMINAL";
        case InterpretationRegime::ELEVATED_VOLATILITY: return "ELEVATED_VOLATILITY";
        case InterpretationRegime::MEMPOOL_CONGESTION: return "MEMPOOL_CONGESTION";
        case InterpretationRegime::TEMPORAL_DRIFT: return "TEMPORAL_DRIFT";
        case InterpretationRegime::CRITICAL_ISOLATION: return "CRITICAL_ISOLATION";
    }
    return "UNKNOWN";
}

} // namespace ailee::interpretation
