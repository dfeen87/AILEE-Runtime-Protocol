#include "l6/ReplayExport.h"
#include "l6/FederationExport.h"
#include "kernel/Hooks.h"
#include <sstream>
#include <iomanip>

namespace ailee {
namespace l6 {

namespace {

// Deterministic state hash for scheduler
std::string hash_scheduler_state(const l4::DeterministicSchedulerState& state) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&state);
    uint64_t hash = 5381;
    for (size_t i = 0; i < sizeof(l4::DeterministicSchedulerState); ++i) {
        hash = ((hash << 5) + hash) + data[i]; // hash * 33 + c
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

std::string format_telemetry(const l4::TelemetrySample& t) {
    std::string out = "{";
    out += "\"consistent_state_root_nodes\":" + std::to_string(t.consistent_state_root_nodes) + ",";
    out += "\"epoch_height\":" + std::to_string(t.epoch_height) + ",";
    out += "\"global_coherence_score\":" + std::to_string(t.global_coherence_score) + ",";
    out += "\"in_sync_nodes\":" + std::to_string(t.in_sync_nodes) + ",";
    out += "\"inconsistent_state_root_nodes\":" + std::to_string(t.inconsistent_state_root_nodes) + ",";
    out += "\"tick_count\":" + std::to_string(t.tick_count) + ",";
    out += "\"total_nodes\":" + std::to_string(t.total_nodes);
    out += "}";
    return out;
}

} // anonymous namespace

ExternalReplayTick ReplayExport::export_tick(
    uint64_t tick_index,
    const l4::ReplayTick& tick
) {
    ExternalReplayTick ext_tick;
    ext_tick.tick_index = tick_index;
    ext_tick.scheduler_state_hash = hash_scheduler_state(tick.scheduler_state);
    ext_tick.telemetry_json = format_telemetry(tick.telemetry);

    FederationExport fed_export;
    ext_tick.cluster_view = fed_export.export_view(tick.view);

    // V35 Extensions:
    ext_tick.replay_phase = "execution";
    ext_tick.replay_mode_state = "active";

    // Trigger early kernel hook
    kernel::ReplayTickContext ctx;
    ctx.tick_index = tick_index;
    ctx.status = "success";
    kernel::Hooks::onReplayTick(ctx, ext_tick);

    return ext_tick;
}

} // namespace l6
} // namespace ailee
