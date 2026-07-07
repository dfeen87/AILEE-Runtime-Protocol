#pragma once

#include <cstdint>
#include <vector>
#include "l4/ClusterTypes.h"
#include "l4/MeshAnchor.h"

namespace ailee {
namespace l4 {

struct alignas(64) StateRootAnnouncement {
    uint64_t source_node_id_hash;  // 8 bytes
    uint64_t epoch_height;         // 8 bytes
    uint8_t state_root[32];        // 32 bytes
    uint8_t announcement_hash[32]; // 32 bytes

    // Total size = 8 + 8 + 32 + 32 = 80 bytes
    // Next multiple of 64 is 128 bytes. Padding needed: 48 bytes
    uint8_t padding[48];
};
static_assert(sizeof(StateRootAnnouncement) == 128, "StateRootAnnouncement must be 128 bytes");

struct alignas(64) StateRootValidationResult {
    uint64_t node_id_hash;         // 8 bytes
    uint64_t epoch_height;         // 8 bytes
    bool accepted;                 // 1 byte
    bool rejected;                 // 1 byte
    uint8_t reason_code;           // 1 byte
    // Total size = 8 + 8 + 1 + 1 + 1 = 19 bytes
    // Next multiple of 64 is 64 bytes. Padding needed: 45 bytes
    uint8_t padding[45];
};
static_assert(sizeof(StateRootValidationResult) == 64, "StateRootValidationResult must be 64 bytes");

std::vector<StateRootAnnouncement> build_state_root_announcements(const ClusterView& view);

std::vector<StateRootValidationResult> validate_state_roots(
    const ClusterView& view,
    const MeshAnchor& anchor,
    const std::vector<StateRootAnnouncement>& announcements
);

} // namespace l4
} // namespace ailee
