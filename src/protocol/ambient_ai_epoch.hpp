#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace ailee {
namespace protocol {

// 32‑byte hash type used throughout the protocol spine
using Hash256 = std::array<uint8_t, 32>;

// ---------------------------------------------------------
// AmbientAIAnchoringEpoch
// ---------------------------------------------------------
// This struct represents the deterministic commitment for an
// Ambient AI epoch. The commitment is serialized and hashed
// into the Taproot tree.
// ---------------------------------------------------------
struct AmbientAIAnchoringEpoch {
    uint64_t epochId = 0;
    uint64_t startBitcoinHeight = 0;
    uint64_t endBitcoinHeight = 0;

    std::vector<uint8_t> l2StateRoot;
    std::vector<uint8_t> energyLedgerRoot;
    std::vector<uint8_t> participationRoot;
    std::vector<uint8_t> ambientEventRoot;
    std::vector<uint8_t> protocolBuildMetadata;

    // Computes the deterministic SHA‑256 commitment for the epoch.
    Hash256 computeEpochCommitment() const;
};

} // namespace protocol
} // namespace ailee
