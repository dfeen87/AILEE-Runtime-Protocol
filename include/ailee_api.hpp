#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Forward declarations of internal context
struct AileeContextImpl;

namespace ailee {

// ============================================================================
// Core Configuration
// ============================================================================

struct AileeConfig {
    std::string environment; // e.g., "production", "testnet", "simulation"
    bool strict_mode;
    // Future extensible fields
};

struct AileeContext {
    AileeContextImpl* impl = nullptr;
};

// ============================================================================
// Input Types
// ============================================================================

struct PriceContext {
    double spot_price;
    double daily_change_pct; // 24h change as a percentage (e.g., 5.2 for +5.2%)
    double volatility_index; // Normalized volatility metric (0.0 to 1.0)
};

struct MempoolStats {
    uint64_t size_bytes;
    uint32_t transaction_count;
    double avg_fee_rate;    // sats/vByte
    double high_fee_band;   // sats/vByte
    double low_fee_band;    // sats/vByte
};

struct BitcoinSnapshot {
    uint32_t height;
    PriceContext price_context;
    MempoolStats mempool_stats;
    double current_fee_rate;            // sats/vByte (median or recommended)
    double hashrate_estimate;           // Smoothed TH/s
    double recent_volatility;           // Normalized metric (0.0 - 1.0)
    std::string dominance_or_regime_tag; // e.g., "risk-on", "risk-off", "chop", "parabolic"
    uint32_t time_since_last_block;     // seconds
    uint32_t block_interval_avg;        // Rolling average over N blocks (seconds)
};

struct Policy {
    double max_acceptable_risk_score;        // e.g., 0.0 - 10.0
    double max_fee_rate_sats_per_vbyte;      // e.g., max fee to pay
    double min_confidence_required;          // e.g., 0.0 - 1.0
    std::vector<std::string> allowed_regimes;// e.g., ["risk-on", "neutral"]
    double block_interval_tolerance_sec;     // e.g., allowed deviation from 600s
};

// ============================================================================
// Output Types
// ============================================================================

enum class RiskBand {
    Low,
    Medium,
    High,
    Extreme
};

struct RiskScore {
    double value;          // 0.0 - 10.0
    RiskBand band;         // Semantic categorization
};

struct PostureReport {
    RiskScore risk;
    std::string regime;    // Evaluated regime
    std::string summary;   // Human-readable summary
    double confidence;     // 0.0 - 1.0
};

struct Advisory {
    PostureReport posture;
    std::string recommended_action; // e.g., "HOLD", "BROADCAST", "ESCALATE_FEE"
    bool action_approved;           // True if policy thresholds are met
};

// ============================================================================
// Core Functions
// ============================================================================

// Initialization
AileeContext ailee_init(const AileeConfig& config);
void ailee_shutdown(AileeContext& context);

// Configuration Loaders
AileeConfig load_config(const std::string& filepath);
Policy load_policy(const std::string& filepath);

// State Evaluation
PostureReport ailee_evaluate_posture(const BitcoinSnapshot& snapshot, const AileeContext& context);
RiskScore ailee_score_risk(const BitcoinSnapshot& snapshot, const AileeContext& context);
Advisory ailee_advise_action(const BitcoinSnapshot& snapshot, const Policy& policy, const AileeContext& context);

} // namespace ailee
