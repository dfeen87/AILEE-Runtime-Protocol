#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ailee::interpretation {

enum class InterpretationRegime {
    NOMINAL,
    ELEVATED_VOLATILITY,
    MEMPOOL_CONGESTION,
    TEMPORAL_DRIFT,
    CRITICAL_ISOLATION
};

struct InterpretationInput {
    double volatility{0.0};
    double fee_rate{0.0};
    double high_fee_band{0.0};
    uint64_t block_lag_seconds{0};
    double coherence_score{1.0};
};

struct InterpretationResult {
    InterpretationRegime regime{InterpretationRegime::NOMINAL};
    std::string regime_name{"NOMINAL"};
    double confidence{1.0};
    std::string recommendation;
    std::vector<std::string> triggered_rules;
};

class InterpretationEngine {
public:
    InterpretationEngine() = default;

    InterpretationResult interpret(const InterpretationInput& input) const;
    static std::string regime_to_string(InterpretationRegime regime);
};

} // namespace ailee::interpretation
