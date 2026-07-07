#include "l2/RecoveryPayload.h"
#include <cstring>
#include <openssl/sha.h>

namespace ailee {
namespace l2 {

RecoveryPayload build_recovery_payload(
    const StateRoot& state_root,
    const EpochState& epoch_state,
    const identity::NodeId& node_identity
) {
    RecoveryPayload payload;
    std::memset(&payload, 0, sizeof(payload));

    std::memcpy(payload.target_state_root, state_root.root_hash, 32);
    std::memcpy(payload.target_epoch_hash, epoch_state.epoch_hash, 32);
    payload.l1_height = epoch_state.l1_height;
    std::memcpy(payload.node_identity_hash, node_identity.id, 32);

    // Deterministically hash the payload contents
    uint8_t buffer[32 + 32 + 8 + 32];
    size_t offset = 0;
    std::memcpy(buffer + offset, payload.target_state_root, 32); offset += 32;
    std::memcpy(buffer + offset, payload.target_epoch_hash, 32); offset += 32;
    std::memcpy(buffer + offset, &payload.l1_height, 8); offset += 8;
    std::memcpy(buffer + offset, payload.node_identity_hash, 32); offset += 32;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256(buffer, offset, payload.payload_hash);
#pragma GCC diagnostic pop

    return payload;
}

} // namespace l2
} // namespace ailee
