#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include "taproot/TaprootScript.h"

namespace ailee {
namespace taproot {

// Deterministic TxOut struct
struct TxOut {
    uint64_t value_sats;
    std::vector<uint8_t> script_pubkey;

    std::vector<uint8_t> to_bytes() const;
};

// Deterministic TxIn struct
struct TxIn {
    std::array<uint8_t, 32> prev_txid;   // little-endian txid
    uint32_t prev_vout;
    std::vector<uint8_t> script_sig;     // empty for unsigned
    uint32_t sequence;
    std::vector<std::vector<uint8_t>> witness;

    std::vector<uint8_t> to_bytes() const;
};

// Deterministic Tx struct
struct Tx {
    uint32_t version;
    std::vector<TxIn> vin;
    std::vector<TxOut> vout;
    uint32_t locktime;

    std::vector<uint8_t> to_bytes() const;
};

// Builds a deterministic P2TR scriptPubKey
std::vector<uint8_t> build_p2tr_script_pubkey(const std::array<uint8_t, 32>& taproot_output_key);

// Builds an unsigned taproot transaction
Tx build_taproot_tx(const std::array<uint8_t, 32>& prev_txid,
                    uint32_t prev_vout,
                    uint64_t input_value_sats,
                    const TaprootOutput& taproot_output,
                    uint64_t output_value_sats,
                    uint32_t locktime);

} // namespace taproot
} // namespace ailee
