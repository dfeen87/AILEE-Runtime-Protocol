#ifndef AMBIENT_AI_MESH_PARTICIPATION_HPP
#define AMBIENT_AI_MESH_PARTICIPATION_HPP

#include <cstdint>
#include <vector>

namespace ailee {
namespace ambient_mesh {

// Summarizes deterministic mesh routing and participation statistics
struct AmbientMeshParticipationSummary {
    uint32_t successfulRoutes;
    uint32_t failedRoutes;
    uint64_t meshUptimeSegments;

    // Serializes strictly into length-prefixed binary form
    std::vector<uint8_t> serialize() const;
};

class MeshParticipationAggregator {
public:
    // Increment deterministic counters triggered by AmbientMeshRoutingEngine
    void recordRoute(bool success);
    void recordUptimeSegment();

    // Finalizes the summary at epoch boundary.
    // This summary directly merges into ailee::ambient_node::AmbientParticipationRecord.
    // This consolidated record then influences the EnergyProfile and L2 Anchoring commitments.
    AmbientMeshParticipationSummary finalizeEpochSummary() const;

    // Resets internal counters at the beginning of a new epoch.
    void resetForNewEpoch();

private:
    AmbientMeshParticipationSummary currentSummary;
};

} // namespace ambient_mesh
} // namespace ailee

#endif // AMBIENT_AI_MESH_PARTICIPATION_HPP
