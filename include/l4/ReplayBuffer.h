#pragma once

#include "l4/ClusterTypes.h"
#include "l4/DeterministicScheduler.h"
#include "l4/DeterministicTelemetry.h"
#include "l5/DeterministicCompressor.h"
#include <vector>

namespace ailee {
namespace l4 {

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
};

struct ReplayBuffer {
    std::vector<ReplaySchedulerSnapshot> scheduler_snapshots;
    std::vector<ReplayClusterViewSnapshot> view_snapshots;
    std::vector<TelemetrySample> telemetry_snapshots;

    std::vector<std::vector<uint8_t>> compressed_ticks;
    l5::DeterministicCompressor compressor;

    void record_tick(
        const DeterministicSchedulerState& scheduler_state,
        const ClusterView& view,
        const TelemetrySample& telemetry_sample
    );
};

} // namespace l4
} // namespace ailee
