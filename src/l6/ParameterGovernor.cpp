#include "l6/ParameterGovernor.h"

namespace ailee::l6 {

ParameterAdjustments adjust_parameters(const SystemMetrics& metrics) {
    ParameterAdjustments params;

    // Baseline values
    params.anchor_interval = 10;
    params.replay_window_size = 32;
    params.payload_size_limit = 4096;
    params.epoch_duration_ms = 60000;

    // Apply deterministic rules (fixed steps from baseline)
    if (metrics.block_interval_ms > 650000) {
        params.anchor_interval += 1;
    }

    if (metrics.mempool_size_bytes > 100 * 1000 * 1000) { // 100 MB
        params.epoch_duration_ms += 1000;
    }

    if (metrics.fee_rate_sat_per_vb > 50) {
        params.payload_size_limit -= 410; // ~10% fixed step reduction
    }

    if (metrics.replay_cost_ms > 2000) {
        params.replay_window_size -= 1;
    }

    if (metrics.epoch_complexity_score > 1000) {
        params.anchor_interval += 1;
    }

    // Enforce bounds
    // anchor_interval: min 5, max 100
    if (params.anchor_interval < 5) params.anchor_interval = 5;
    if (params.anchor_interval > 100) params.anchor_interval = 100;

    // replay_window_size: min 8, max 128
    if (params.replay_window_size < 8) params.replay_window_size = 8;
    if (params.replay_window_size > 128) params.replay_window_size = 128;

    // payload_size_limit: min 1024, max 8192
    if (params.payload_size_limit < 1024) params.payload_size_limit = 1024;
    if (params.payload_size_limit > 8192) params.payload_size_limit = 8192;

    // epoch_duration_ms: min 30000, max 300000
    if (params.epoch_duration_ms < 30000) params.epoch_duration_ms = 30000;
    if (params.epoch_duration_ms > 300000) params.epoch_duration_ms = 300000;

    return params;
}

} // namespace ailee::l6
