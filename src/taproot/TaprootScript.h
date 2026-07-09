#pragma once

#include <array>
#include <vector>
#include <string>
#include <cstdint>

namespace ailee {
namespace taproot {

// Deterministic TapLeaf struct
struct TapLeaf {
    std::array<uint8_t, 32> anchor_root;
    std::vector<uint8_t> script; // serialized Bitcoin script

    // Builds the deterministic TapLeaf script based on anchor_root
    void build_script();
};

// Deterministic TaprootOutput struct
struct TaprootOutput {
    std::array<uint8_t, 32> internal_key;       // x-only pubkey
    std::array<uint8_t, 32> taproot_output_key; // result of tweaking internal_key
    std::string bech32m_address;

    // Computes the tweaked taproot output key (placeholder for Phase 1)
    void compute_output_key(const std::array<uint8_t, 32>& tapleaf_hash);
};

// Computes the BIP342-style tagged hash for the TapLeaf
std::array<uint8_t, 32> compute_tapleaf_hash(const TapLeaf& leaf);

// Returns a Bech32m address (placeholder for Phase 1)
std::string to_bech32m(const std::string& hrp, const std::vector<uint8_t>& witness_program);

// High-level integration helper for constructing a Taproot output from an anchor root
TaprootOutput build_taproot_for_anchor(const std::array<uint8_t, 32>& anchor_root,
                                       const std::array<uint8_t, 32>& internal_key);

} // namespace taproot
} // namespace ailee
