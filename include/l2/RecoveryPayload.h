#pragma once

#include <cstdint>
#include "NodeIdentity.h"
#include "l2/EpochScheduler.h"
#include "l2/StateRootPipeline.h"

namespace ailee {
namespace l2 {

struct alignas(64) RecoveryPayload {
    uint8_t payload_hash[32];
    uint8_t target_state_root[32];
    uint8_t target_epoch_hash[32];
    uint64_t l1_height;
    uint8_t node_identity_hash[32];
    uint8_t padding[56]; // 32*4 + 8 + 56 = 192 bytes total
};
static_assert(sizeof(RecoveryPayload) == 192, "RecoveryPayload must be 192 bytes");

struct alignas(64) RecoverySummary {
    uint8_t recovery_hash[32];
    uint64_t payload_size_bytes; // abstract size
    bool requires_state_sync;
    uint8_t padding[23]; // 32 + 8 + 1 + 23 = 64 bytes
};
static_assert(sizeof(RecoverySummary) == 64, "RecoverySummary must be 64 bytes");

RecoveryPayload build_recovery_payload(
    const StateRoot& state_root,
    const EpochState& epoch_state,
    const identity::NodeId& node_identity
);

} // namespace l2
} // namespace ailee
