#pragma once

#include <vector>
#include <cstdint>

namespace ailee::l6::isla {

struct EpochCoherenceMetrics {
    double dispute_rate;
    uint32_t reorg_alerts;
    double finalization_latency;
    double anchor_confirmation_time;
};

struct ProverPerformanceMetrics {
    double proof_generation_time;
    double prover_utilization;
    double backlog_clearance_rate;
};

struct EconomicFeeMetrics {
    double sat_per_vbyte_fee_band;
    double anchor_cost_ratio;
    double l2_backlog_pressure;
};

struct EpochMetricsWindow {
    std::vector<EpochCoherenceMetrics> epochs;
};

struct PerformanceMetricsWindow {
    std::vector<ProverPerformanceMetrics> epochs;
};

struct EconomicMetricsWindow {
    std::vector<EconomicFeeMetrics> epochs;
};

} // namespace ailee::l6::isla
