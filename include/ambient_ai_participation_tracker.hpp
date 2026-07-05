#ifndef AMBIENT_AI_PARTICIPATION_TRACKER_HPP
#define AMBIENT_AI_PARTICIPATION_TRACKER_HPP

#include <cstdint>
#include <vector>
#include <array>

namespace ailee {
namespace ambient_node {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientParticipationRecord {
    uint32_t successfulRoutes;
    uint32_t failedRoutes;
    uint32_t validSnapshotsTaken;
    uint64_t uptimeSegments; // Count of valid L2 heartbeat ticks

    // Accrues deterministic participation metrics
    void accrueRoute(bool success);
    void accrueSnapshot();
    void accrueUptime();
};

struct AmbientParticipationEpochSummary {
    uint64_t epochId;
    AmbientParticipationRecord record;

    // Binds directly to ailee::energy::EnergyProfile mapping
    Hash256 computeParticipationHash() const;
    std::vector<uint8_t> serializeForEnergyProfile() const;
};

class AmbientParticipationTracker {
public:
    // Tracks ongoing participation during the current epoch
    void update(const AmbientParticipationRecord& update);

    // Finalizes the epoch summary for ingestion into the anchoring layer
    AmbientParticipationEpochSummary finalizeEpoch(uint64_t epochId) const;

    // Clears the internal state for the new epoch
    void resetForNewEpoch();
};

} // namespace ambient_node
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_TRACKER_HPP
