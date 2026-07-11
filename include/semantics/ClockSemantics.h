#pragma once

#include "l6/IslaRuntimeOrchestrator.h"
#include "semantics/SemanticsTypes.h"

namespace ailee::semantics {

struct ClockSemantics {
    static bool is_valid_snapshot(const l6::ClockSnapshot& snapshot, const l6::IReplayBuffer& replay_buffer, EnvironmentType env);
    static bool is_epoch_boundary(const l6::ClockSnapshot& snapshot, uint32_t blocks_per_epoch);
};

} // namespace ailee::semantics
