#include "l4/ReplayBuffer.h"
#include "l6/JsonBindings.h"

namespace ailee {
namespace l4 {

void ReplayBuffer::record_tick(
    const DeterministicSchedulerState& scheduler_state,
    const ClusterView& view,
    const TelemetrySample& telemetry_sample
) {
    ReplayTick tick;
    tick.scheduler_state = scheduler_state;
    tick.view = view;
    tick.telemetry = telemetry_sample;
    tick.height = 0; // The real tick gets constructed properly later or these defaults apply for recording. Note: cluster view should probably have clock/replay_events if it's set correctly.
    tick.clock = view.clock;
    tick.replay_events = view.replay_events;
    auto compressed = compressor.compress_tick(view, tick);
    compressed_ticks.push_back(std::move(compressed));

    auto external_tick = replay_export.export_tick(scheduler_snapshots.size(), tick);
    auto json = l6::JsonBindings::to_json(external_tick);
    external_json_ticks.push_back(json);

    scheduler_snapshots.push_back({ scheduler_state });
    
    ReplayClusterViewSnapshot snap;
    
    // Perform true deep copies of contents, not pointers
    snap.nodes.reserve(view.nodes.size());
    for (const auto& node : view.nodes) {
        snap.nodes.push_back(node);
    }
    
    snap.mesh_envelopes = view.mesh_envelopes;
    snap.transport_queue = view.transport_queue;
    snap.total_nodes = view.total_nodes;
    snap.total_steps = view.total_steps;
    snap.coherence_summary = view.coherence_summary;
    snap.clock = view.clock;
    snap.replay_events = view.replay_events;
    
    view_snapshots.push_back(std::move(snap));
    
    telemetry_snapshots.push_back(telemetry_sample);
}

} // namespace l4
} // namespace ailee
