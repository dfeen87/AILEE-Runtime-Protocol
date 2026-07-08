#pragma once

#include "l4/ReplayBuffer.h"
#include "l1_sync/replay_input.hpp"
#include "l4/ReplayTick.h"
#include <string>

namespace ailee {
namespace l4 {

struct ReplayEngine {
    ReplayBuffer buffer;

    ReplayTick step(const l1_sync::ReplayState& previous, const l1_sync::ReplayInput& input) const;

    void load_replay_file(const std::string& path);
    void write_replay_file(const std::string& path, const ReplayBuffer& replay_buffer) const;

    bool replay_tick(size_t tick_index,
                     DeterministicSchedulerState& out_scheduler,
                     ClusterView& out_view,
                     TelemetrySample& out_telemetry) const;

    bool verify_tick(size_t tick_index,
                     const DeterministicSchedulerState& scheduler,
                     const ClusterView& view,
                     const TelemetrySample& telemetry) const;
};

} // namespace l4
} // namespace ailee
