#pragma once

#include <cstdint>
#include "NodeIdentity.h"
#include "ReflectionLayer.h"

namespace ailee {
namespace l2 {

// Deterministic Epoch State
struct alignas(64) EpochState {
    uint8_t epoch_hash[32];
    uint64_t epoch_number;
    uint64_t l1_height;
    uint8_t l1_anchor_hash[32];
    uint32_t protocol_version;
    uint8_t node_identity_hash[32];
    uint32_t padding; // Ensure 64-byte alignment and no uninitialized padding
};
static_assert(sizeof(EpochState) == 128, "EpochState must be a multiple of 64 bytes");

// Transition context
struct alignas(64) EpochTransition {
    uint8_t previous_epoch_hash[32];
    uint8_t new_epoch_hash[32];
    uint64_t transition_l1_height;
    uint8_t padding[56]; // padding to 128 bytes
};
static_assert(sizeof(EpochTransition) == 128, "EpochTransition must be 128 bytes");

// Schedule summary
struct alignas(64) EpochScheduleSummary {
    uint8_t current_epoch_hash[32];
    uint64_t next_scheduled_l1_height;
    uint32_t protocol_version_active;
    uint8_t padding[20]; // 32 + 8 + 4 + 20 = 64
};
static_assert(sizeof(EpochScheduleSummary) == 64, "EpochScheduleSummary must be 64 bytes");

// Pure reproducible function to compute epoch state
EpochState compute_epoch_state(
    const reflection::ReflectionSnapshot& reflection_snapshot,
    const identity::NodeId& node_identity,
    uint32_t protocol_version
);

} // namespace l2
} // namespace ailee
