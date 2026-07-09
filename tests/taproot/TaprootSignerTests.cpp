#include "../../src/taproot/TaprootSigner.h"
#include <gtest/gtest.h>
#include <secp256k1.h>
#include <array>
#include <vector>
#include <cstring>

using namespace ailee::taproot;

TEST(TaprootSignerTest, SignAndAttach) {
    // Basic keys
    std::array<uint8_t, 32> internal_key;
    std::fill(internal_key.begin(), internal_key.end(), 0x11);

    std::array<uint8_t, 32> taproot_output_key;
    std::fill(taproot_output_key.begin(), taproot_output_key.end(), 0x22);

    std::array<uint8_t, 32> privkey;
    std::fill(privkey.begin(), privkey.end(), 0x01); // Simple priv key for testing secp256k1 validation

    // Build unsigned tx
    std::array<uint8_t, 32> prev_txid;
    std::fill(prev_txid.begin(), prev_txid.end(), 0xAA);

    TaprootOutput t_out;
    t_out.internal_key = internal_key;
    t_out.taproot_output_key = taproot_output_key;

    Tx tx = build_taproot_tx(prev_txid, 0, 1000, t_out, 500, 0);

    // Sign
    Tx signed_tx = build_signed_taproot_tx(tx, internal_key, taproot_output_key, privkey, 1000);

    EXPECT_EQ(signed_tx.vin.size(), 1);
    EXPECT_EQ(signed_tx.vin[0].witness.size(), 1); // Should have attached 1 witness item
    EXPECT_EQ(signed_tx.vin[0].witness[0].size(), 64); // Schnorr sig is 64 bytes

    // Check Segwit Serialization
    std::vector<uint8_t> bytes = signed_tx.to_bytes();
    EXPECT_GT(bytes.size(), 0);
    EXPECT_EQ(bytes[4], 0x00); // Segwit marker
    EXPECT_EQ(bytes[5], 0x01); // Segwit flag
}
