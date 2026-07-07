#include "l4/DeterministicTelemetry.h"
#include "l4/DeterministicScheduler.h" // For DeterministicSchedulerState
#include <cstring>

namespace ailee {
namespace l4 {

void record_telemetry_sample(
    const ClusterView& view,
    const DeterministicSchedulerState& scheduler_state,
    TelemetryBuffer& buffer)
{
    TelemetrySample sample = {};
    std::memset(&sample, 0, sizeof(sample));

    sample.tick_count = scheduler_state.tick_count;
    sample.epoch_height = scheduler_state.epoch_height;
    sample.total_nodes = view.total_nodes;

    // Use pure observation as requested
    sample.in_sync_nodes = view.coherence_summary.in_sync_count;
    sample.consistent_state_root_nodes = view.coherence_summary.consistent_state_root_nodes;
    sample.inconsistent_state_root_nodes = view.coherence_summary.inconsistent_state_root_nodes;
    sample.global_coherence_score = view.coherence_summary.global_coherence_score;

    buffer.samples.push_back(sample);
}

} // namespace l4
} // namespace ailee
