#include "l2/EpochScheduler.h"
#include <cstring>
#include <openssl/sha.h>

namespace ailee {
namespace l2 {

EpochState compute_epoch_state(
    const reflection::ReflectionSnapshot& reflection_snapshot,
    const identity::NodeId& node_identity,
    uint32_t protocol_version
) {
    EpochState state;
    std::memset(&state, 0, sizeof(state));

    state.l1_height = reflection_snapshot.height.height;
    std::memcpy(state.l1_anchor_hash, reflection_snapshot.anchor.anchor_hash, 32);
    state.protocol_version = protocol_version;
    std::memcpy(state.node_identity_hash, node_identity.id, 32);

    // Deterministic progression rule:
    // Epoch number advances per L1 height interval or directly tracks it.
    state.epoch_number = state.l1_height;
    state.padding = 0;

    // Hash the components together to form the epoch_hash deterministically
    uint8_t buffer[8 + 8 + 32 + 4 + 32];
    size_t offset = 0;
    std::memcpy(buffer + offset, &state.epoch_number, 8); offset += 8;
    std::memcpy(buffer + offset, &state.l1_height, 8); offset += 8;
    std::memcpy(buffer + offset, state.l1_anchor_hash, 32); offset += 32;
    std::memcpy(buffer + offset, &state.protocol_version, 4); offset += 4;
    std::memcpy(buffer + offset, state.node_identity_hash, 32); offset += 32;

    // Use OpenSSL SHA256 one-shot to avoid dynamic allocation from EVP
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256(buffer, offset, state.epoch_hash);
#pragma GCC diagnostic pop

    return state;
}

} // namespace l2
} // namespace ailee
