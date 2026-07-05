#ifndef AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP
#define AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace ailee {
namespace participation {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic categories of ambient contributions
enum class ContributionType : uint8_t {
    ROUTING_SUCCESS = 0,    // Successful micro-route propagation
    SENSING_SNAPSHOT = 1,   // Valid privacy-preserving signal snapshot
    UPTIME_SEGMENT = 2      // Verified low-power node availability segment
};

// Privacy-preserving discrete buckets for sensing events
struct AmbientContributionBucket {
    uint8_t powerStateClass;     // e.g., 0=Battery, 1=Charging, 2=Mains
    uint8_t connectivityBand;    // e.g., 0=LowBandwidth, 1=HighBandwidth
    uint8_t activityLevel;       // Deterministically discretized activity index

    // Serializes to a strictly bounded byte array
    std::vector<uint8_t> serialize() const;
};

// Represents a single, deterministic contribution event logged by a node
struct AmbientContributionEvent {
    Hash256 eventId;                 // Hash of local context (must not contain sensitive data)
    uint64_t logicalTimestamp;       // Protocol-defined logical tick (NOT wall-clock)
    ContributionType type;
    AmbientContributionBucket bucket;

    // Binary serialization for deterministic aggregation
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP
