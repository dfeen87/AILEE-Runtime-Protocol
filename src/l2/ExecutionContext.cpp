#include "l2/ExecutionContext.h"
#include <cstring>
#include <openssl/sha.h>

namespace ailee {
namespace l2 {

ExecutionContext build_execution_context(
    const identity::NodeId& node_identity,
    const EpochState& epoch_state,
    const StateRoot& state_root,
    const mesh::MeshCoherenceResult& coherence
) {
    ExecutionContext context;
    std::memset(&context, 0, sizeof(context));

    std::memcpy(context.epoch_hash, epoch_state.epoch_hash, 32);
    std::memcpy(context.state_root_hash, state_root.root_hash, 32);
    std::memcpy(context.node_identity_hash, node_identity.id, 32);
    context.l1_height = epoch_state.l1_height;
    context.mesh_coherence_score = coherence.score;

    // Hash components into context_hash
    uint8_t buffer[32 + 32 + 32 + 8 + 4];
    size_t offset = 0;
    std::memcpy(buffer + offset, context.epoch_hash, 32); offset += 32;
    std::memcpy(buffer + offset, context.state_root_hash, 32); offset += 32;
    std::memcpy(buffer + offset, context.node_identity_hash, 32); offset += 32;
    std::memcpy(buffer + offset, &context.l1_height, 8); offset += 8;
    std::memcpy(buffer + offset, &context.mesh_coherence_score, 4); offset += 4;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256(buffer, offset, context.context_hash);
#pragma GCC diagnostic pop

    return context;
}

} // namespace l2
} // namespace ailee
