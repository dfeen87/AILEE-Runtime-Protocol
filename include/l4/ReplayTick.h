#pragma once

#include <vector>
#include <cstdint>
#include "l4/DeterministicScheduler.h"
#include "l4/ClusterTypes.h"
#include "l4/DeterministicTelemetry.h"

namespace ailee {
namespace l4 {

struct ReplayTick {
    DeterministicSchedulerState scheduler_state;
    ClusterView view;
    TelemetrySample telemetry;
    uint64_t height;
    l1_sync::BitcoinClockState clock;
    std::vector<l1_sync::ReplayEvent> replay_events;

    std::vector<uint8_t> serialize() const;
    static ReplayTick deserialize(const std::vector<uint8_t>& raw);
};

} // namespace l4
} // namespace ailee
