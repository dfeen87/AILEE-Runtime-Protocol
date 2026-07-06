#pragma once

#include "ambient_ai_consensus_state_machine.hpp"
#include "../../wnn/state_machine/wave_epoch_phase_state_machine.hpp"

namespace ailee {
namespace core {
namespace consensus {

class WnnConsensusIntegration {
public:
    // PhaseStateMachine -> AmbientConsensusStateMachine
    bool transition_consensus_on_phase(
        const ailee::wnn::PhaseStateMachine& phaseMachine,
        ailee::consensus::AmbientConsensusStateMachine& stateMachine,
        uint64_t logicalTimestamp
    ) const;
};

} // namespace consensus
} // namespace core
} // namespace ailee
