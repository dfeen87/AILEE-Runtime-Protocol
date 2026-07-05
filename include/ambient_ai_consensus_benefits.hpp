#ifndef AMBIENT_AI_CONSENSUS_BENEFITS_HPP
#define AMBIENT_AI_CONSENSUS_BENEFITS_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace ailee {
namespace consensus {

struct AmbientConsensusReputationSignal {
    std::string publicKeyHex;
    int64_t energyScore;
    bool isParticipationVerified;

    std::vector<uint8_t> serialize() const;
};

struct AmbientConsensusBenefit {
    std::string publicKeyHex;

    uint32_t routingPriorityMultiplier;

    uint32_t relayPriorityMultiplier;

    void calculateFromReputation(const AmbientConsensusReputationSignal& signal);
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_CONSENSUS_BENEFITS_HPP
