#include "ambient_ai_l2_merkleization.hpp"
#include "ambient_ai_epoch.hpp"
#include <openssl/sha.h>
#include <string.h>

namespace ailee {
namespace consensus {

Hash256 AmbientL2Merkleizer::computeMerkleRoot(const std::vector<Hash256>& leaves) const {
    if (leaves.empty()) {
        Hash256 emptyRoot = {0};
        return emptyRoot;
    }

    std::vector<Hash256> current_level = leaves;
    while (current_level.size() > 1) {
        std::vector<Hash256> next_level;
        for (size_t i = 0; i < current_level.size(); i += 2) {
            if (i + 1 < current_level.size()) {
                std::vector<uint8_t> combined;
                combined.insert(combined.end(), current_level[i].begin(), current_level[i].end());
                combined.insert(combined.end(), current_level[i+1].begin(), current_level[i+1].end());
                Hash256 result;
                SHA256(combined.data(), combined.size(), result.data());
                next_level.push_back(result);
            } else {
                next_level.push_back(current_level[i]);
            }
        }
        current_level = next_level;
    }

    return current_level[0];
}

protocol::AmbientAIAnchoringEpoch AmbientL2Merkleizer::buildEpochCommitment(
    uint64_t epochId,
    uint64_t startHeight,
    uint64_t endHeight,
    const Hash256& l2StateRoot,
    const Hash256& energyLedgerRoot,
    const Hash256& participationRoot,
    const Hash256& ambientEventRoot
) const {
    protocol::AmbientAIAnchoringEpoch epoch;
    epoch.epochId = epochId;
    epoch.startBitcoinHeight = startHeight;
    epoch.endBitcoinHeight = endHeight;
    epoch.l2StateRoot = l2StateRoot;
    epoch.energyLedgerRoot = energyLedgerRoot;
    epoch.participationRoot = participationRoot;
    epoch.ambientEventRoot = ambientEventRoot;

    return epoch;
}

}
}
