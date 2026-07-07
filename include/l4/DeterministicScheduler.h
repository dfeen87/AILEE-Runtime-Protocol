#pragma once

#include <cstdint>
#include <vector>
#include <utility>
#include <cstring>
#include "l4/ClusterTypes.h"
#include "l4/MeshAnchor.h"
#include "l4/StateRootPropagation.h"
#include "l2/DeterministicEngine.h"
#include "l4/ClusterSim.h"
#include "l4/DeterministicTelemetry.h"

namespace ailee {
namespace l4 {

enum class SchedulerPhase : uint8_t {
    ENGINE_STEP = 0,
    GOSSIP_PEER_SYNC = 1,
    RECOVERY = 2,
    MESH_EPOCH_BUILD = 3,
    MESH_ANCHOR_BUILD = 4,
    STATE_ROOT_ANNOUNCEMENTS = 5,
    STATE_ROOT_VALIDATION = 6,
    TRANSPORT_DELIVERY = 7,
    COHERENCE_UPDATE = 8
};

struct alignas(64) DeterministicSchedulerState {
    uint64_t tick_count;
    uint64_t epoch_height;
    uint64_t last_anchor_id;
    uint64_t last_mesh_state_root_hash;
    bool cluster_stable;
    uint8_t padding[31];
};
static_assert(sizeof(DeterministicSchedulerState) == 64, "DeterministicSchedulerState must be 64 bytes");

struct DeterministicScheduler {
    DeterministicSchedulerState state;

    // Temporary storage across phases within a full cycle
    MeshEpoch current_epoch;
    MeshAnchor current_anchor;
    std::vector<StateRootAnnouncement> current_announcements;

    TelemetryBuffer telemetry;

    DeterministicScheduler() {
        std::memset(&state, 0, sizeof(state));
        std::memset(&current_epoch, 0, sizeof(current_epoch));
        std::memset(&current_anchor, 0, sizeof(current_anchor));
    }

    void run_tick(
        ClusterView& view,
        const std::vector<std::pair<size_t, size_t>>& gossip_schedule,
        std::vector<l2::DeterministicEngine>& engines
    );
};

} // namespace l4
} // namespace ailee
