#pragma once

#include <cstdint>
#include <string>
#include "l6/IslaRuntimeOrchestrator.h"
#include "semantics/SemanticsTypes.h"

namespace ailee::semantics {

struct EpochSemantics {
    static uint64_t compute_epoch_id(const l6::ClockSnapshot& snapshot, uint32_t blocks_per_epoch);
    static bool validate_epoch_bundle(const l6::EpochIntegrationBundle& bundle, const l6::IReplayBuffer& replay_buffer, uint32_t blocks_per_epoch);
};

} // namespace ailee::semantics
