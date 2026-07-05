#ifndef AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP
#define AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP

#include <cstdint>
#include <string>

namespace ailee {
namespace participation {

// Encapsulates a coordination and routing advantage signal
struct AmbientReputationSignal {
    std::string publicKeyHex;
    uint32_t reliabilityTier;    // Deterministically bucketed (e.g., 1 to 5)
    uint64_t cumulativeUptime;   // Sourced from multi-epoch participation records
};

// A hint used by L2 coordination nodes to bias L1 relay and L2 mesh routing
struct AmbientGiveBackHint {
    AmbientReputationSignal signal;

    // Deterministic routing boost applied to ReputationRateLimiter.
    // e.g., higher reputation yields a larger forward rate multiplier.
    int32_t calculateRoutingMultiplier() const;

    // Generates a priority score used to prefer this node as a relay candidate
    // for anchoring Bitcoin L1 transactions.
    uint32_t calculateL1RelayPriority() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP
