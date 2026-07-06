#include "ailee_core_anchoring.hpp"

namespace ailee {
namespace core {
namespace anchoring {

ailee::protocol::AmbientAIAnchoringEpoch WnnAnchoringIntegration::anchor_wave_state(
    const ailee::wnn::WaveStateCommitment& stateCommitment,
    const ailee::consensus::AmbientL2Merkleizer& merkleizer,
    uint64_t epochId,
    uint64_t startHeight,
    uint64_t endHeight,
    const std::vector<ailee::consensus::Hash256>& leaves
) const {
    // Generate the baseline L2 Ambient Event Root
    ailee::consensus::Hash256 ambientEventRoot = merkleizer.computeMerkleRoot(leaves);

    // In a deterministic implementation, stateCommitment data is hashed and incorporated
    // into the tree structure.
    ailee::consensus::Hash256 emptyRoot = {0};

    return merkleizer.buildEpochCommitment(
        epochId,
        startHeight,
        endHeight,
        emptyRoot,
        emptyRoot,
        emptyRoot,
        ambientEventRoot
    );
}

} // namespace anchoring
} // namespace core
} // namespace ailee
