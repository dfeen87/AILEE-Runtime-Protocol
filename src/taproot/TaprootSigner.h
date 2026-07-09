#pragma once

#include "taproot/TaprootTxBuilder.h"
#include <array>
#include <string>
#include <cstdint>

namespace ailee {
namespace taproot {

// Deterministic signature result
struct TaprootSignatureResult {
    bool success;
    std::array<uint8_t, 64> signature;   // BIP340 Schnorr signature
    std::string error;                   // empty on success
};

// Deterministic signing function
TaprootSignatureResult sign_taproot_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
);

// Attaches the Taproot witness to the transaction
bool attach_taproot_witness(
    Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 64>& signature
);

// Builds the final signed Taproot transaction
Tx build_signed_taproot_tx(
    const Tx& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
);

} // namespace taproot
} // namespace ailee
