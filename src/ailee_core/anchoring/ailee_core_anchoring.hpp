#pragma once

#include "ambient_ai_l2_merkleization.hpp"
#include "ambient_ai_epoch.hpp"
#include <vector>
#include <cstdint>

namespace ailee {
namespace wnn {
struct WaveStateCommitment {
    std::vector<uint8_t> data;
};
}
}

namespace ailee {
namespace core {
namespace anchoring {

class WnnAnchoringIntegration {
public:
    // WaveStateCommitment -> AmbientL2Merkleizer
    ailee::protocol::AmbientAIAnchoringEpoch anchor_wave_state(
        const ailee::wnn::WaveStateCommitment& stateCommitment,
        const ailee::consensus::AmbientL2Merkleizer& merkleizer,
        uint64_t epochId,
        uint64_t startHeight,
        uint64_t endHeight,
        const std::vector<ailee::consensus::Hash256>& leaves
    ) const;
};

} // namespace anchoring
} // namespace core
} // namespace ailee
