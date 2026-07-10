#include "anchor/AnchorCommitValidator.h"
#include "taproot/TaprootScript.h"
#include "taproot/TaprootTxBuilder.h"
#include "taproot/TaprootUtils.h"

#include <openssl/sha.h>
#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_extrakeys.h>

#include <cstring>
#include <algorithm>

namespace ailee {
namespace anchor {

namespace {

// Tagged hash: SHA256(SHA256(tag) || SHA256(tag) || msg)
void tagged_hash(const char* tag, const std::vector<uint8_t>& msg, std::array<uint8_t, 32>& out_hash) {
    std::array<uint8_t, 32> tag_hash;
    SHA256(reinterpret_cast<const unsigned char*>(tag), std::strlen(tag), tag_hash.data());

    std::vector<uint8_t> buffer;
    buffer.reserve(32 + 32 + msg.size());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), msg.begin(), msg.end());

    SHA256(buffer.data(), buffer.size(), out_hash.data());
}

// Helper to read a little-endian 32-bit integer
uint32_t read_uint32_le(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) return 0;
    uint32_t val = static_cast<uint32_t>(data[offset]) |
                   (static_cast<uint32_t>(data[offset + 1]) << 8) |
                   (static_cast<uint32_t>(data[offset + 2]) << 16) |
                   (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return val;
}

// Helper to read a little-endian 64-bit integer
uint64_t read_uint64_le(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 8 > data.size()) return 0;
    uint64_t val = static_cast<uint64_t>(data[offset]) |
                   (static_cast<uint64_t>(data[offset + 1]) << 8) |
                   (static_cast<uint64_t>(data[offset + 2]) << 16) |
                   (static_cast<uint64_t>(data[offset + 3]) << 24) |
                   (static_cast<uint64_t>(data[offset + 4]) << 32) |
                   (static_cast<uint64_t>(data[offset + 5]) << 40) |
                   (static_cast<uint64_t>(data[offset + 6]) << 48) |
                   (static_cast<uint64_t>(data[offset + 7]) << 56);
    offset += 8;
    return val;
}

// Helper to read a Bitcoin CompactSize (varint)
uint64_t read_compact_size(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset >= data.size()) return 0;
    uint8_t first = data[offset++];
    if (first < 253) {
        return first;
    } else if (first == 253) {
        if (offset + 2 > data.size()) return 0;
        uint64_t val = data[offset] | (data[offset + 1] << 8);
        offset += 2;
        return val;
    } else if (first == 254) {
        if (offset + 4 > data.size()) return 0;
        uint64_t val = read_uint32_le(data, offset);
        return val;
    } else {
        if (offset + 8 > data.size()) return 0;
        uint64_t val = read_uint64_le(data, offset);
        return val;
    }
}

} // anonymous namespace

AnchorCommitValidationResult AnchorCommitValidator::validate_anchor_commit(
    const std::vector<uint8_t>& signed_tx,
    const std::array<uint8_t, 32>& anchor_root,
    const std::array<uint8_t, 32>& metadata_hash,
    const std::array<uint8_t, 32>& internal_key,
    uint64_t value_sats
) {
    AnchorCommitValidationResult result;
    result.ok = false;

    if (signed_tx.size() < 60) {
        result.error = "tx-too-short";
        return result;
    }

    size_t offset = 0;
    uint32_t version = read_uint32_le(signed_tx, offset);
    if (version != 2) {
        result.error = "invalid-version";
        return result;
    }

    if (offset + 2 > signed_tx.size()) {
        result.error = "tx-too-short";
        return result;
    }

    uint8_t marker = signed_tx[offset++];
    uint8_t flag = signed_tx[offset++];
    if (marker != 0x00 || flag != 0x01) {
        result.error = "not-segwit-tx";
        return result;
    }

    uint64_t input_count = read_compact_size(signed_tx, offset);
    if (input_count != 1) {
        result.error = "invalid-input-count";
        return result;
    }

    // Parse the single input
    if (offset + 36 > signed_tx.size()) {
        result.error = "tx-too-short";
        return result;
    }

    std::array<uint8_t, 32> prev_txid;
    std::copy(signed_tx.begin() + offset, signed_tx.begin() + offset + 32, prev_txid.begin());
    offset += 32;

    uint32_t prev_vout = read_uint32_le(signed_tx, offset);

    uint64_t script_sig_len = read_compact_size(signed_tx, offset);
    if (script_sig_len != 0) {
        result.error = "non-empty-script-sig";
        return result;
    }

    if (offset + 4 > signed_tx.size()) {
        result.error = "tx-too-short";
        return result;
    }
    uint32_t sequence = read_uint32_le(signed_tx, offset);

    uint64_t output_count = read_compact_size(signed_tx, offset);
    if (output_count != 1) {
        result.error = "invalid-output-count";
        return result;
    }

    // Parse the single output
    if (offset + 8 > signed_tx.size()) {
        result.error = "tx-too-short";
        return result;
    }
    uint64_t output_value = read_uint64_le(signed_tx, offset);

    uint64_t script_pubkey_len = read_compact_size(signed_tx, offset);
    if (offset + script_pubkey_len > signed_tx.size()) {
        result.error = "tx-too-short";
        return result;
    }

    std::vector<uint8_t> output_script_pubkey;
    output_script_pubkey.insert(output_script_pubkey.end(),
                                signed_tx.begin() + offset,
                                signed_tx.begin() + offset + script_pubkey_len);
    offset += script_pubkey_len;

    // Witness data for input 0
    uint64_t witness_count = read_compact_size(signed_tx, offset);
    if (witness_count != 1) {
        result.error = "invalid-witness-count";
        return result;
    }

    uint64_t witness_item_len = read_compact_size(signed_tx, offset);
    if (witness_item_len != 64) {
        result.error = "invalid-signature-length";
        return result;
    }

    if (offset + 64 > signed_tx.size()) {
        result.error = "tx-too-short";
        return result;
    }

    std::array<uint8_t, 64> signature;
    std::copy(signed_tx.begin() + offset, signed_tx.begin() + offset + 64, signature.begin());
    offset += 64;

    if (offset + 4 > signed_tx.size()) {
        result.error = "tx-too-short";
        return result;
    }
    uint32_t locktime = read_uint32_le(signed_tx, offset);
    if (locktime != 0) {
        result.error = "invalid-locktime";
        return result;
    }

    // Rebuild TapLeaf script
    taproot::AnchorCommitTapLeaf tapleaf = taproot::TaprootScript::build_anchor_commit_tapleaf(
        anchor_root, metadata_hash);

    // Recompute tweaked key
    taproot::TaprootOutput taproot_output;
    taproot_output.internal_key = internal_key;
    taproot_output.compute_output_key(tapleaf.leaf_hash);

    // Verify scriptPubKey matches
    std::vector<uint8_t> expected_script_pubkey = taproot::build_p2tr_script_pubkey(taproot_output.taproot_output_key);
    if (output_script_pubkey != expected_script_pubkey) {
        result.error = "scriptpubkey-mismatch";
        return result;
    }

    // Build Sighash
    std::vector<uint8_t> sha_prevouts_input;
    sha_prevouts_input.insert(sha_prevouts_input.end(), prev_txid.begin(), prev_txid.end());
    taproot::write_uint32_le(sha_prevouts_input, prev_vout);
    std::array<uint8_t, 32> sha_prevouts;
    SHA256(sha_prevouts_input.data(), sha_prevouts_input.size(), sha_prevouts.data());

    std::vector<uint8_t> sha_amounts_input;
    taproot::write_uint64_le(sha_amounts_input, value_sats);
    std::array<uint8_t, 32> sha_amounts;
    SHA256(sha_amounts_input.data(), sha_amounts_input.size(), sha_amounts.data());

    std::vector<uint8_t> sha_scriptpubkeys_input;
    taproot::write_compact_size(sha_scriptpubkeys_input, expected_script_pubkey.size());
    sha_scriptpubkeys_input.insert(sha_scriptpubkeys_input.end(), expected_script_pubkey.begin(), expected_script_pubkey.end());
    std::array<uint8_t, 32> sha_scriptpubkeys;
    SHA256(sha_scriptpubkeys_input.data(), sha_scriptpubkeys_input.size(), sha_scriptpubkeys.data());

    std::vector<uint8_t> sha_sequences_input;
    taproot::write_uint32_le(sha_sequences_input, sequence);
    std::array<uint8_t, 32> sha_sequences;
    SHA256(sha_sequences_input.data(), sha_sequences_input.size(), sha_sequences.data());

    std::vector<uint8_t> sha_outputs_input;
    taproot::write_uint64_le(sha_outputs_input, output_value);
    taproot::write_compact_size(sha_outputs_input, output_script_pubkey.size());
    sha_outputs_input.insert(sha_outputs_input.end(), output_script_pubkey.begin(), output_script_pubkey.end());
    std::array<uint8_t, 32> sha_outputs;
    SHA256(sha_outputs_input.data(), sha_outputs_input.size(), sha_outputs.data());

    std::vector<uint8_t> sighash_msg;
    sighash_msg.push_back(0x00); // epoch
    sighash_msg.push_back(0x00); // hash_type (SIGHASH_DEFAULT)

    taproot::write_uint32_le(sighash_msg, version);
    taproot::write_uint32_le(sighash_msg, locktime);

    sighash_msg.insert(sighash_msg.end(), sha_prevouts.begin(), sha_prevouts.end());
    sighash_msg.insert(sighash_msg.end(), sha_amounts.begin(), sha_amounts.end());
    sighash_msg.insert(sighash_msg.end(), sha_scriptpubkeys.begin(), sha_scriptpubkeys.end());
    sighash_msg.insert(sighash_msg.end(), sha_sequences.begin(), sha_sequences.end());
    sighash_msg.insert(sighash_msg.end(), sha_outputs.begin(), sha_outputs.end());

    sighash_msg.push_back(0x00); // spend_type = 0
    taproot::write_uint32_le(sighash_msg, 0); // input_index = 0

    std::array<uint8_t, 32> sighash;
    tagged_hash("TapSighash", sighash_msg, sighash);

    // Verify Signature
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    if (!ctx) {
        result.error = "secp256k1-context-failed";
        return result;
    }

    secp256k1_xonly_pubkey verification_pubkey;
    if (secp256k1_xonly_pubkey_parse(ctx, &verification_pubkey, taproot_output.taproot_output_key.data()) != 1) {
        result.error = "invalid-verification-key";
        secp256k1_context_destroy(ctx);
        return result;
    }

    if (secp256k1_schnorrsig_verify(ctx, signature.data(), sighash.data(), 32, &verification_pubkey) != 1) {
        result.error = "invalid-signature";
        secp256k1_context_destroy(ctx);
        return result;
    }

    secp256k1_context_destroy(ctx);

    result.ok = true;
    return result;
}

} // namespace anchor
} // namespace ailee
