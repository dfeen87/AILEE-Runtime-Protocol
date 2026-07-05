#ifndef AMBIENT_AI_L2_CONSENSUS_MODEL_HPP
#define AMBIENT_AI_L2_CONSENSUS_MODEL_HPP

#include <vector>
#include <array>
#include <cstdint>

namespace ailee {
namespace protocol {
    struct AmbientAIAnchoringEpoch;
}
namespace identity {
    struct ParticipationProof;
    struct IdentityPayload;
}
namespace energy {
    struct EnergyProfile;
    struct EnergyLedgerEntry;
}
namespace ambient {
    struct AmbientEvent;
}
namespace ambient_mesh {
    struct AmbientMeshParticipationSummary;
}
}

namespace ailee {
namespace consensus {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientL2ConsensusState {
    uint64_t currentEpochId;
    uint64_t currentLogicalTimestamp;

    Hash256 l2StateRoot;
    Hash256 energyLedgerRoot;
    Hash256 participationRoot;
    Hash256 ambientEventRoot;

    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

struct AmbientL2ConsensusView {
    AmbientL2ConsensusState stateSnapshot;

    bool verifyInvariants() const;
    Hash256 getSnapshotHash() const;
};

struct AmbientL2EpochBoundary {
    uint64_t previousEpochId;
    uint64_t newEpochId;
    uint64_t targetBitcoinHeight;

    protocol::AmbientAIAnchoringEpoch finalizePreviousEpoch(const AmbientL2ConsensusState& currentState) const;
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_L2_CONSENSUS_MODEL_HPP
