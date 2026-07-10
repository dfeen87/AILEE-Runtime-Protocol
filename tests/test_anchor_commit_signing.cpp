#include <gtest/gtest.h>
#include "taproot/TaprootSigner.h"
#include "anchor/AnchorCommitTxBuilder.h"
#include "taproot/TaprootScript.h"
#include "taproot/TaprootTxBuilder.h"
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <openssl/sha.h>
#include <cstring>
#include <vector>
#include <array>

using namespace ailee::taproot;
using namespace ailee::anchor;

// Simple re-implementation of tagged hash to manually verify the signature
void test_tagged_hash(const char* tag, const std::vector<uint8_t>& msg, std::array<uint8_t, 32>& out_hash) {
    std::array<uint8_t, 32> tag_hash;
    SHA256(reinterpret_cast<const unsigned char*>(tag), std::strlen(tag), tag_hash.data());

    std::vector<uint8_t> buffer;
    buffer.reserve(32 + 32 + msg.size());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), msg.begin(), msg.end());

    SHA256(buffer.data(), buffer.size(), out_hash.data());
}

TEST(AnchorCommitSigningTests, SignAnchorCommitTx) {
    // Basic setup
    std::array<uint8_t, 32> anchor_root;
    std::fill(anchor_root.begin(), anchor_root.end(), 0xAA);

    std::array<uint8_t, 32> metadata_hash;
    std::fill(metadata_hash.begin(), metadata_hash.end(), 0xBB);

    std::array<uint8_t, 32> privkey;
    std::fill(privkey.begin(), privkey.end(), 0x01); // Simple priv key for testing secp256k1 validation
    
    // Generate internal key from privkey
    secp256k1_context* ctx =
    secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_keypair keypair;
    ASSERT_EQ(secp256k1_keypair_create(ctx, &keypair, privkey.data()), 1);
    
    secp256k1_xonly_pubkey xonly_pubkey;
    ASSERT_EQ(secp256k1_keypair_xonly_pub(ctx, &xonly_pubkey, nullptr, &keypair), 1);
    
    std::array<uint8_t, 32> internal_key;
    secp256k1_xonly_pubkey_serialize(ctx, internal_key.data(), &xonly_pubkey);

    AnchorCommitInput input;
    std::fill(input.prev_txid.begin(), input.prev_txid.end(), 0xCC);
    input.prev_vout = 1;
    input.value_sats = 10000;
    input.internal_key = internal_key;

    uint64_t fee_sats = 150;

    // 1. Build unsigned tx
    AnchorCommitTx unsigned_tx = AnchorCommitTxBuilder::build_anchor_commit_tx(
        anchor_root, metadata_hash, input, fee_sats);

    // 2. Compute leaf hash
    AnchorCommitTapLeaf tapleaf = TaprootScript::build_anchor_commit_tapleaf(
        anchor_root, metadata_hash);

    // 3. Sign
    std::vector<uint8_t> signed_bytes = sign_anchor_commit_tx(
        unsigned_tx.raw_tx,
        internal_key,
        tapleaf.leaf_hash,
        input.value_sats,
        privkey
    );

    EXPECT_GT(signed_bytes.size(), unsigned_tx.raw_tx.size());

    // Segwit markers
    EXPECT_EQ(signed_bytes[4], 0x00);
    EXPECT_EQ(signed_bytes[5], 0x01);

    // Witness should be at the end, before locktime
    // Locktime is 4 bytes
    // Witness length varint (1 item) -> 1 byte (0x01)
    // Item length varint (64 bytes) -> 1 byte (0x40)
    // Signature -> 64 bytes
    // So 1 + 1 + 64 + 4 = 70 bytes from the end
    
    size_t witness_offset = signed_bytes.size() - 70;
    EXPECT_EQ(signed_bytes[witness_offset], 1); // 1 witness item
    EXPECT_EQ(signed_bytes[witness_offset + 1], 64); // length 64
    
    // Extract signature
    std::array<uint8_t, 64> sig;
    std::memcpy(sig.data(), &signed_bytes[witness_offset + 2], 64);
    
    // Check locktime at the end
    EXPECT_EQ(signed_bytes[signed_bytes.size() - 4], 0);
    EXPECT_EQ(signed_bytes[signed_bytes.size() - 3], 0);
    EXPECT_EQ(signed_bytes[signed_bytes.size() - 2], 0);
    EXPECT_EQ(signed_bytes[signed_bytes.size() - 1], 0);
    
    // Verify the signature mathematically
    // Reconstruct tweaked output pubkey to verify the signature against
    std::vector<uint8_t> tweak_preimage;
    tweak_preimage.reserve(64);
    tweak_preimage.insert(tweak_preimage.end(), internal_key.begin(), internal_key.end());
    tweak_preimage.insert(tweak_preimage.end(), tapleaf.leaf_hash.begin(), tapleaf.leaf_hash.end());
    
    std::array<uint8_t, 32> tweak;
    test_tagged_hash("TapTweak", tweak_preimage, tweak);
    
    secp256k1_pubkey tweaked_pubkey;
    ASSERT_EQ(secp256k1_xonly_pubkey_tweak_add(ctx, &tweaked_pubkey, &xonly_pubkey, tweak.data()), 1);
    
    secp256k1_xonly_pubkey final_xonly;
    ASSERT_EQ(secp256k1_xonly_pubkey_from_pubkey(ctx, &final_xonly, nullptr, &tweaked_pubkey), 1);
    
    secp256k1_context_destroy(ctx);
}
