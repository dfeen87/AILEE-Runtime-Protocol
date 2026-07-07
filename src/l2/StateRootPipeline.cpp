#include "l2/StateRootPipeline.h"
#include <cstring>
#include <openssl/sha.h>

namespace ailee {
namespace l2 {

StateRoot compute_state_root(
    const reflection::ReflectionSnapshot& reflection,
    const l1::SettlementIngestion& ingestion,
    const mesh::MeshCoherenceResult& coherence
) {
    StateRoot root;
    std::memset(&root, 0, sizeof(root));

    root.l1_height_basis = reflection.height.height;
    root.ingestion_count = ingestion.latest_confirmations; // as an arbitrary deterministic mix-in
    root.coherence_score = coherence.score;

    // Hash components
    uint8_t buffer[8 + 32 + 4 + 4];
    size_t offset = 0;
    std::memcpy(buffer + offset, &root.l1_height_basis, 8); offset += 8;
    std::memcpy(buffer + offset, reflection.anchor.anchor_hash, 32); offset += 32;
    std::memcpy(buffer + offset, &root.ingestion_count, 4); offset += 4;
    std::memcpy(buffer + offset, &root.coherence_score, 4); offset += 4;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    SHA256(buffer, offset, root.root_hash);
#pragma GCC diagnostic pop

    return root;
}

} // namespace l2
} // namespace ailee
