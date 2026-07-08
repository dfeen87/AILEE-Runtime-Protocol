#pragma once

#include "l4/ClusterTypes.h"
#include "l4/DeterministicTelemetry.h"
#include "l5/DeterministicCompressor.h"
#include "l6/ReplayExport.h"
#include <vector>
#include <string>

namespace ailee {
namespace l4 {

// Include full definition since we instantiate it inside ReplaySchedulerSnapshot
#include "l4/DeterministicScheduler.h"

struct alignas(64) ReplaySchedulerSnapshot {
    DeterministicSchedulerState state; // 64 bytes
};
static_assert(sizeof(ReplaySchedulerSnapshot) == 64, "ReplaySchedulerSnapshot must be 64 bytes");

struct ReplayClusterViewSnapshot {
    std::vector<ClusterNodeState> nodes;
    std::vector<MeshPropagationEnvelope> mesh_envelopes;
    TransportQueue transport_queue;
    uint64_t total_nodes;
    uint64_t total_steps;
    ClusterCoherenceSummary coherence_summary;
    l1_sync::BitcoinClockState clock;
    std::vector<l1_sync::ReplayEvent> replay_events;
};

struct ReplayBuffer {
    std::vector<ReplaySchedulerSnapshot> scheduler_snapshots;
    std::vector<ReplayClusterViewSnapshot> view_snapshots;
    std::vector<TelemetrySample> telemetry_snapshots;

    std::vector<std::vector<uint8_t>> compressed_ticks;
    l5::DeterministicCompressor compressor;

    l6::ReplayExport replay_export;
    std::vector<std::string> external_json_ticks;

    void record_tick(
        const DeterministicSchedulerState& scheduler_state,
        const ClusterView& view,
        const TelemetrySample& telemetry_sample
    );
};

} // namespace l4
} // namespace ailee
