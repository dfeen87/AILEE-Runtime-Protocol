#ifndef AMBIENT_AI_EPOCH_HPP
#define AMBIENT_AI_EPOCH_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace ailee {
namespace protocol {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic constants for energy calculus (Scale = 10000)
constexpr int64_t FIXED_POINT_SCALE = 10000;
constexpr int64_t ALPHA_COMPUTE = 6000; // 0.6
constexpr int64_t BETA_UPTIME   = 3000; // 0.3
constexpr int64_t GAMMA_STORAGE = 1000; // 0.1

struct AmbientAIAnchoringEpoch {
    uint64_t epochId;               // Monotonically increasing L2 epoch counter
    uint64_t startBitcoinHeight;    // L1 block height where epoch begins
    uint64_t endBitcoinHeight;      // L1 block height where epoch ends (inclusive)

    Hash256 l2StateRoot;            // Deterministic state root of the L2 Ledger, Bridge, and Orchestrator
    Hash256 energyLedgerRoot;       // Merkle root of all node energy balances (See ambient_ai_primitives_spec.md)
    Hash256 participationRoot;      // Merkle root of deterministic node identity/participation proofs (See ambient_ai_primitives_spec.md)
    Hash256 ambientEventRoot;       // Merkle root of the Ambient Internet extension event tree (See ambient_ai_primitives_spec.md)

    Hash256 protocolBuildMetadata;  // Hash of the deterministic BuildInfo (commit, version)

    // The final payload to be serialized and committed into the Taproot tree.
    // The resulting commitment forms a single TapLeaf within the TapTree.
    Hash256 computeEpochCommitment() const;
};

} // namespace protocol
} // namespace ailee

#endif // AMBIENT_AI_EPOCH_HPP
