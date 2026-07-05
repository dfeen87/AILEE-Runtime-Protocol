#ifndef AMBIENT_AI_L2_MERKLEIZATION_HPP
#define AMBIENT_AI_L2_MERKLEIZATION_HPP

#include <vector>
#include <array>
#include <cstdint>

namespace ailee {
namespace protocol {
    struct AmbientAIAnchoringEpoch;
}
namespace anchoring {
    class TaprootAnchor;
}
}

namespace ailee {
namespace consensus {

using Hash256 = std::array<uint8_t, 32>;

class AmbientL2Merkleizer {
public:
    Hash256 computeMerkleRoot(const std::vector<Hash256>& leaves) const;

    protocol::AmbientAIAnchoringEpoch buildEpochCommitment(
        uint64_t epochId,
        uint64_t startHeight,
        uint64_t endHeight,
        const Hash256& l2StateRoot,
        const Hash256& energyLedgerRoot,
        const Hash256& participationRoot,
        const Hash256& ambientEventRoot
    ) const;
};

} // namespace consensus
} // namespace ailee

#endif // AMBIENT_AI_L2_MERKLEIZATION_HPP
