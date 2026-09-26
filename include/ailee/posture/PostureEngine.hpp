#pragma once

#include <string>
#include <cstdint>

namespace ailee::posture {

enum class PostureRegime {
    NEUTRAL,
    PARABOLIC,
    RISK_OFF,
    CHOP,
    STRESS,
    RECOVERY
};

struct PostureEvaluationInput {
    double current_fee_rate{0.0};
    double high_fee_band{0.0};
    double recent_volatility{0.0};
    uint64_t block_interval_avg{600};
    uint64_t time_since_last_block{0};
    double daily_change_pct{0.0};
    double signal_coherence{1.0};
};

struct PostureResult {
    double risk_score{0.0}; // 0.0 to 10.0
    PostureRegime regime{PostureRegime::NEUTRAL};
    std::string regime_id{"neutral"};
    std::string summary;
    double confidence{1.0};
    double temporal_coherence_index{1.0};
};

class PostureEngine {
public:
    PostureEngine() = default;

    PostureResult evaluate(const PostureEvaluationInput& input) const;
    static std::string regime_to_string(PostureRegime regime);
};

} // namespace ailee::posture
