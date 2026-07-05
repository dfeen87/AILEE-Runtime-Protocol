#include "ambient_ai_epoch.hpp"
#include <cstdint>
#include <array>
#ifndef AMBIENT_AI_TAPROOT_ANCHOR_HPP
#define AMBIENT_AI_TAPROOT_ANCHOR_HPP

#include <vector>

namespace ailee {
namespace anchoring {

// Uses libsecp256k1 for all taproot tweak computations
class TaprootAnchor {
public:
    // Internal federation public key (Aggregate key)
    std::array<uint8_t, 32> internalPubKey;

    // Builds the TapTree leaves
    // Leaf 0: L2 Epoch Commitment
    // Leaf 1: Optional BitVM-style challenge response script
    struct TapTreeState {
        std::vector<uint8_t> controlBlock;
        std::array<uint8_t, 32> tweakedPubKey;
        std::vector<uint8_t> script; // OP_RETURN or OP_FALSE OP_IF ... OP_ENDIF
    };

    // Computes the exact Taproot tweak needed to commit the epoch
    TapTreeState buildEpochCommitment(
        const protocol::AmbientAIAnchoringEpoch& epoch
    ) const;

    // Verifies an existing Taproot output contains our commitment
    bool verifyCommitment(
        const std::array<uint8_t, 32>& outputPubKey,
        const protocol::AmbientAIAnchoringEpoch& expectedEpoch
    ) const;
};

} // namespace anchoring
} // namespace ailee

#endif // AMBIENT_AI_TAPROOT_ANCHOR_HPP
