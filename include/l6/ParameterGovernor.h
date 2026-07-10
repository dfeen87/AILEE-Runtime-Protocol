#pragma once

#include <cstdint>

namespace ailee::l6 {

struct SystemMetrics {
    uint64_t block_interval_ms;       // average Bitcoin block time
    uint64_t mempool_size_bytes;      // current mempool size
    uint64_t fee_rate_sat_per_vb;     // median fee rate
    uint64_t anchor_latency_ms;       // time to anchor last epoch
    uint64_t replay_cost_ms;          // time to verify last replay
    uint64_t epoch_complexity_score;  // deterministic complexity metric
};

struct ParameterAdjustments {
    uint64_t anchor_interval;         // how often to anchor
    uint64_t replay_window_size;      // how many epochs to replay
    uint64_t payload_size_limit;      // max anchor payload size
    uint64_t epoch_duration_ms;       // deterministic epoch duration
};

ParameterAdjustments adjust_parameters(const SystemMetrics& metrics);

} // namespace ailee::l6
