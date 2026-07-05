#ifndef AMBIENT_AI_CONSENSUS_STATE_MACHINE_HPP
#define AMBIENT_AI_CONSENSUS_STATE_MACHINE_HPP

#include <cstdint>

namespace ailee {
namespace consensus {

enum class ConsensusPhase : uint8_t {
    INIT = 0,
    COLLECT = 1,
    ORDER = 2,
    MERKLEIZE = 3,
    ANCHOR = 4,
    FINALIZE = 5,
    RECOVERY = 6
};

struct AmbientConsensusTransition {
    ConsensusPhase fromPhase;
    ConsensusPhase toPhase;
    uint64_t logicalTimestamp;

    bool isValidTransition() const;
};

class AmbientConsensusStateMachine {
public:
    AmbientConsensusStateMachine();

    ConsensusPhase getCurrentPhase() const;

    bool advancePhase(const AmbientConsensusTransition& transition);

    bool initiateRecovery();

private:
    ConsensusPhase currentPhase;

    bool validateCollectPhaseInvariants() const;
    bool validateOrderPhaseInvariants() const;
    bool validateMerkleizePhaseInvariants() const;
    bool validateAnchorPhaseInvariants() const;
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_CONSENSUS_STATE_MACHINE_HPP
