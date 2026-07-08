#pragma once

#include <cstdint>

namespace ailee {
namespace l4 {

struct alignas(64) DeterministicSchedulerState {
    uint64_t tick_count;
    uint64_t epoch_height;
    uint64_t last_anchor_id;
    uint64_t last_mesh_state_root_hash;
    bool cluster_stable;
    uint8_t padding[31];
};
static_assert(sizeof(DeterministicSchedulerState) == 64, "DeterministicSchedulerState must be 64 bytes");

} // namespace l4
} // namespace ailee
