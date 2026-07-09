#pragma once

#include <vector>
#include <array>
#include <cstdint>

namespace ailee {
namespace anchor {

// Deterministic input struct for AnchorCommitTx
struct AnchorCommitInput {
    std::array<uint8_t, 32> prev_txid;    // little-endian txid
    uint32_t prev_vout;                   // output index
    uint64_t value_sats;                  // input value in satoshis
    std::array<uint8_t, 32> internal_key; // x-only pubkey
};

// Deterministic output struct representing raw unsigned transaction bytes
struct AnchorCommitTx {
    std::vector<uint8_t> raw_tx;
};

// Builder for deterministic unsigned Taproot anchor-commit transactions
class AnchorCommitTxBuilder {
public:
    static AnchorCommitTx build_anchor_commit_tx(
        const std::array<uint8_t, 32>& anchor_root,
        const std::array<uint8_t, 32>& metadata_hash,
        const AnchorCommitInput& input,
        uint64_t fee_sats
    );
};

} // namespace anchor
} // namespace ailee
