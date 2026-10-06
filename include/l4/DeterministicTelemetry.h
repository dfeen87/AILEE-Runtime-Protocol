#pragma once

#include <cstdint>
#include <vector>
#include "l4/ClusterTypes.h"
#include "l4/DeterministicSchedulerState.h"

namespace ailee {
namespace l4 {

struct alignas(64) TelemetrySample {
    uint64_t tick_count = 0;
    uint64_t epoch_height = 0;
    uint64_t total_nodes = 0;
    uint64_t in_sync_nodes = 0;
    uint64_t consistent_state_root_nodes = 0;
    uint64_t inconsistent_state_root_nodes = 0;
    uint64_t global_coherence_score = 0;
    uint8_t padding[8] = {0}; // pad to 64 bytes
};
static_assert(sizeof(TelemetrySample) == 64, "TelemetrySample must be 64 bytes");

struct TelemetryBuffer {
    std::vector<TelemetrySample> samples;
};

void record_telemetry_sample(
    const ClusterView& view,
    const DeterministicSchedulerState& scheduler_state,
    TelemetryBuffer& buffer);

} // namespace l4
} // namespace ailee
