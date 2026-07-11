#pragma once

#include "l6/IslaRuntimeOrchestrator.h"

namespace ailee::semantics {

struct ReplaySemantics {
    static bool validate_replay_chain(const l6::IReplayBuffer& buffer, uint32_t max_replay_epochs);
    static void enforce_replay_retention(l6::IReplayBuffer& buffer, uint32_t max_replay_epochs);
};

} // namespace ailee::semantics
