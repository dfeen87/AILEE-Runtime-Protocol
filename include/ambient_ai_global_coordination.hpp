#ifndef AMBIENT_AI_GLOBAL_COORDINATION_HPP
#define AMBIENT_AI_GLOBAL_COORDINATION_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace ailee {
namespace runtime {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientCoordinationHint {
    std::string peerId;
    int64_t energyScore; // Derived directly from scaled EnergyProfile logic
    uint32_t routingPriorityMultiplier;
    uint32_t relayPriorityMultiplier;

    // Binary serialization strictly bounded and deterministic
    std::vector<uint8_t> serialize() const;
};

struct AmbientCoordinationState {
    uint64_t currentLogicalTimestamp;
    std::vector<AmbientCoordinationHint> networkHints;

    // Generates a deterministic hash representing the collective routing landscape
    Hash256 stateHash() const;
};

class AmbientCoordinationFabric {
public:
    // Derives hints purely from the consensus energy profile array
    std::vector<AmbientCoordinationHint> generateHintsFromEnergy(
        const std::vector<AmbientCoordinationHint>& previousHints
    ) const;

    // Updates the global mesh reputation signals, impacting rate limits globally
    AmbientCoordinationState updateCoordinationState(
        const std::vector<AmbientCoordinationHint>& newHints,
        uint64_t logicalTimestamp
    );
};

} // namespace runtime
} // namespace ailee

#endif // AMBIENT_AI_GLOBAL_COORDINATION_HPP
