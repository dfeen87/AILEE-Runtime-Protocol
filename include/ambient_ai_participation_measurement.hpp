#ifndef AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP
#define AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP

#include <cstdint>
#include <vector>
#include <set>
#include <array>
#include "ambient_ai_participation_contributions.hpp"

namespace ailee {
namespace participation {

// Canonical limits to prevent overflow and spam attacks
struct ParticipationLimits {
    static constexpr uint32_t MAX_EVENTS_PER_EPOCH = 5000;
    static constexpr uint32_t MAX_SENSING_PER_TICK = 1;
};

// Summarizes a full epoch's worth of contribution events deterministically
struct AmbientContributionSummary {
    uint32_t totalRoutingSuccesses;
    uint32_t totalSensingSnapshots;
    uint32_t totalUptimeSegments;

    // Serializes into a canonical length-prefixed binary array
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

// Deterministic event logger enforcing rate-limits and deduplication
class AmbientContributionLog {
public:
    // Attempts to log an event. Deterministically drops events that exceed:
    // - MAX_EVENTS_PER_EPOCH
    // - Duplicate eventId (replay protection)
    // - Rate limits (e.g., MAX_SENSING_PER_TICK)
    bool logEvent(const AmbientContributionEvent& event);

    // Finalizes the epoch's valid events into a bounded summary.
    // Clears the internal log state for the next epoch.
    AmbientContributionSummary finalizeEpochSummary();

private:
    std::vector<AmbientContributionEvent> validEvents;

    // Custom comparator for deterministic ordering of Hashes
    struct HashCompare {
        bool operator()(const Hash256& a, const Hash256& b) const;
    };
    std::set<Hash256, HashCompare> seenEventIds;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP
