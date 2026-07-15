#include <gtest/gtest.h>
#include "anchor/AnchorCommitValidator.h"
#include "anchor/AnchorCommitTxBuilder.h"
#include "taproot/TaprootSigner.h"
#include "taproot/TaprootScript.h"
#include <openssl/sha.h>
#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_extrakeys.h>
#include <array>
#include <vector>

using namespace ailee::anchor;
using namespace ailee::taproot;

class AnchorCommitValidatorTest : public ::testing::Test {
protected:
    std::array<uint8_t, 32> anchor_root;
    std::array<uint8_t, 32> metadata_hash;
    std::array<uint8_t, 32> privkey;
    std::array<uint8_t, 32> internal_key;
    uint64_t value_sats;

public:
    void SetUp() override {
        // Set deterministic values for testing
        anchor_root.fill(0xAA);
        metadata_hash.fill(0xBB);
        privkey.fill(0x01); // Just a valid private key
        value_sats = 100000;

        // Derive internal key from privkey
        secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
        secp256k1_keypair keypair;
        int res1 = secp256k1_keypair_create(ctx, &keypair, privkey.data());
        (void)res1;
        secp256k1_xonly_pubkey xonly;
        int res2 = secp256k1_keypair_xonly_pub(ctx, &xonly, nullptr, &keypair);
        (void)res2;
        secp256k1_xonly_pubkey_serialize(ctx, internal_key.data(), &xonly);
        secp256k1_context_destroy(ctx);
    }

    std::vector<uint8_t> create_valid_signed_tx(
        const std::array<uint8_t, 32>& a_root,
        const std::array<uint8_t, 32>& m_hash
    ) {
        AnchorCommitInput input;
        input.internal_key = internal_key;
        input.prev_txid.fill(0x11);
        input.prev_vout = 0;
        input.value_sats = value_sats;

        AnchorCommitTx unsigned_tx = AnchorCommitTxBuilder::build_anchor_commit_tx(
            a_root, m_hash, input, 1000);

        AnchorCommitTapLeaf tapleaf = TaprootScript::build_anchor_commit_tapleaf(
            a_root, m_hash);

        return sign_anchor_commit_tx(
            unsigned_tx.raw_tx,
            internal_key,
            tapleaf.leaf_hash,
            value_sats,
            privkey
        );
    }
};

TEST_F(AnchorCommitValidatorTest, ValidAnchorCommitPasses) {
    std::vector<uint8_t> signed_tx = create_valid_signed_tx(anchor_root, metadata_hash);

    AnchorCommitValidationResult result = AnchorCommitValidator::validate_anchor_commit(
        signed_tx, anchor_root, metadata_hash, internal_key, value_sats);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.error, "");
}

TEST_F(AnchorCommitValidatorTest, TamperedMetadataHashFails) {
    std::vector<uint8_t> signed_tx = create_valid_signed_tx(anchor_root, metadata_hash);

    std::array<uint8_t, 32> tampered_metadata_hash = metadata_hash;
    tampered_metadata_hash[0] ^= 0xFF; // Flip bits

    AnchorCommitValidationResult result = AnchorCommitValidator::validate_anchor_commit(
        signed_tx, anchor_root, tampered_metadata_hash, internal_key, value_sats);

    EXPECT_FALSE(result.ok);
    // Either scriptpubkey-mismatch or invalid-signature depending on where validation fails first
    EXPECT_NE(result.error, "");
}

TEST_F(AnchorCommitValidatorTest, TamperedAnchorRootFails) {
    std::vector<uint8_t> signed_tx = create_valid_signed_tx(anchor_root, metadata_hash);

    std::array<uint8_t, 32> tampered_anchor_root = anchor_root;
    tampered_anchor_root[31] ^= 0xFF; // Flip bits

    AnchorCommitValidationResult result = AnchorCommitValidator::validate_anchor_commit(
        signed_tx, tampered_anchor_root, metadata_hash, internal_key, value_sats);

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error, "");
}

TEST_F(AnchorCommitValidatorTest, TamperedSignatureFails) {
    std::vector<uint8_t> signed_tx = create_valid_signed_tx(anchor_root, metadata_hash);

    // Tamper with the signature (last 64 bytes of the transaction)
    EXPECT_TRUE(signed_tx.size() > 64);
    signed_tx[signed_tx.size() - 5] ^= 0x01;

    AnchorCommitValidationResult result = AnchorCommitValidator::validate_anchor_commit(
        signed_tx, anchor_root, metadata_hash, internal_key, value_sats);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "invalid-signature");
}

TEST_F(AnchorCommitValidatorTest, WrongTransactionStructureFails) {
    std::vector<uint8_t> signed_tx = create_valid_signed_tx(anchor_root, metadata_hash);

    // Mess up the version
    signed_tx[0] = 0x03;

    AnchorCommitValidationResult result = AnchorCommitValidator::validate_anchor_commit(
        signed_tx, anchor_root, metadata_hash, internal_key, value_sats);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "invalid-version");
}
