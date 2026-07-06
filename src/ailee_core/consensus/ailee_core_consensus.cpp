#include "ailee_core_consensus.hpp"

namespace ailee {
namespace core {
namespace consensus {

bool WnnConsensusIntegration::transition_consensus_on_phase(
    const ailee::wnn::PhaseStateMachine& phaseMachine,
    ailee::consensus::AmbientConsensusStateMachine& stateMachine,
    uint64_t logicalTimestamp
) const {
    ailee::consensus::AmbientConsensusTransition transition;
    transition.fromPhase = stateMachine.getCurrentPhase();

    // Bind WNN Phase State Machine to L2 State Machine
    if (phaseMachine.is_locked()) {
        transition.toPhase = ailee::consensus::ConsensusPhase::ORDER;
    } else {
        transition.toPhase = ailee::consensus::ConsensusPhase::RECOVERY;
    }

    transition.logicalTimestamp = logicalTimestamp;
    return stateMachine.advancePhase(transition);
}

} // namespace consensus
} // namespace core
} // namespace ailee
