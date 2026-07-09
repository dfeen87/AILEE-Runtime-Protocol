#include "taproot/TaprootSigner.h"
#include "taproot/TaprootScript.h"
#include "taproot/TaprootUtils.h"
#include <openssl/sha.h>
#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_extrakeys.h>
#include <cstring>
#include <vector>

namespace ailee {
namespace taproot {

namespace {

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

} // anonymous namespace

TaprootSignatureResult sign_taproot_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
) {
    TaprootSignatureResult result;
    result.success = false;
    std::fill(result.signature.begin(), result.signature.end(), 0);

    if (input_index >= tx.vin.size()) {
        result.error = "invalid-input-index";
        return result;
    }

    // --- 1. Compute BIP341 sighash ---

    // We only have one input according to scope
    std::vector<uint8_t> prevouts_data;
    for (const auto& in : tx.vin) {
        prevouts_data.insert(prevouts_data.end(), in.prev_txid.begin(), in.prev_txid.end());
        write_uint32_le(prevouts_data, in.prev_vout);
    }
    std::array<uint8_t, 32> sha_prevouts;
    SHA256(prevouts_data.data(), prevouts_data.size(), sha_prevouts.data());

    std::vector<uint8_t> amounts_data;
    // Assuming single input logic as specified, we only know the value_sats for this input.
    write_uint64_le(amounts_data, value_sats);
    std::array<uint8_t, 32> sha_amounts;
    SHA256(amounts_data.data(), amounts_data.size(), sha_amounts.data());

    std::vector<uint8_t> spks_data;
    std::vector<uint8_t> script_pubkey = build_p2tr_script_pubkey(taproot_output_key);
    write_compact_size(spks_data, script_pubkey.size());
    spks_data.insert(spks_data.end(), script_pubkey.begin(), script_pubkey.end());
    std::array<uint8_t, 32> sha_scriptpubkeys;
    SHA256(spks_data.data(), spks_data.size(), sha_scriptpubkeys.data());

    std::vector<uint8_t> sequences_data;
    for (const auto& in : tx.vin) {
        write_uint32_le(sequences_data, in.sequence);
    }
    std::array<uint8_t, 32> sha_sequences;
    SHA256(sequences_data.data(), sequences_data.size(), sha_sequences.data());

    std::vector<uint8_t> outputs_data;
    for (const auto& out : tx.vout) {
        write_uint64_le(outputs_data, out.value_sats);
        write_compact_size(outputs_data, out.script_pubkey.size());
        outputs_data.insert(outputs_data.end(), out.script_pubkey.begin(), out.script_pubkey.end());
    }
    std::array<uint8_t, 32> sha_outputs;
    SHA256(outputs_data.data(), outputs_data.size(), sha_outputs.data());

    // Assemble sighash message
    std::vector<uint8_t> sighash_msg;
    // Epoch: 0x00
    sighash_msg.push_back(0x00);
    // hash_type (SIGHASH_DEFAULT)
    sighash_msg.push_back(0x00);
    // nVersion
    write_uint32_le(sighash_msg, tx.version);
    // nLockTime
    write_uint32_le(sighash_msg, tx.locktime);

    // hashes
    sighash_msg.insert(sighash_msg.end(), sha_prevouts.begin(), sha_prevouts.end());
    sighash_msg.insert(sighash_msg.end(), sha_amounts.begin(), sha_amounts.end());
    sighash_msg.insert(sighash_msg.end(), sha_scriptpubkeys.begin(), sha_scriptpubkeys.end());
    sighash_msg.insert(sighash_msg.end(), sha_sequences.begin(), sha_sequences.end());

    if (!(0x00 & 0x01) && !(0x00 & 0x02)) { // SIGHASH_ALL (SIGHASH_DEFAULT maps to SIGHASH_ALL semantics for outputs)
        sighash_msg.insert(sighash_msg.end(), sha_outputs.begin(), sha_outputs.end());
    }

    // spend_type = 0 (no annex, no script path)
    sighash_msg.push_back(0x00);

    // input index
    write_uint32_le(sighash_msg, static_cast<uint32_t>(input_index));

    std::array<uint8_t, 32> sighash;
    tagged_hash("TapSighash", sighash_msg, sighash);

    // --- 2. Produce Schnorr signature ---

    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!ctx) {
        result.error = "secp256k1-context-failed";
        return result;
    }

    secp256k1_keypair keypair;
    if (secp256k1_keypair_create(ctx, &keypair, privkey.data()) != 1) {
        result.error = "invalid-private-key";
        secp256k1_context_destroy(ctx);
        return result;
    }

    // Tweak the private key with TapTweak
    std::vector<uint8_t> internal_key_vec(internal_key.begin(), internal_key.end());
    std::array<uint8_t, 32> tweak;
    tagged_hash("TapTweak", internal_key_vec, tweak);

    if (secp256k1_keypair_xonly_tweak_add(ctx, &keypair, tweak.data()) != 1) {
        result.error = "tweak-failed";
        std::fill(result.signature.begin(), result.signature.end(), 0);
        secp256k1_context_destroy(ctx);
        return result;
    }

    if (secp256k1_schnorrsig_sign(ctx, result.signature.data(), sighash.data(), &keypair, nullptr) != 1) {
        result.error = "schnorr-sign-failed";
        std::fill(result.signature.begin(), result.signature.end(), 0);
        secp256k1_context_destroy(ctx);
        return result;
    }

    secp256k1_context_destroy(ctx);
    result.success = true;
    return result;
}

bool attach_taproot_witness(
    Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 64>& signature
) {
    if (input_index >= tx.vin.size()) {
        return false;
    }

    tx.vin[input_index].script_sig.clear();
    tx.vin[input_index].witness.clear();

    std::vector<uint8_t> sig_vec(signature.begin(), signature.end());
    tx.vin[input_index].witness.push_back(sig_vec);

    return true;
}

Tx build_signed_taproot_tx(
    const Tx& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
) {
    Tx tx = unsigned_tx;

    TaprootSignatureResult sig_result = sign_taproot_input(tx, 0, internal_key, taproot_output_key, privkey, value_sats);
    if (sig_result.success) {
        attach_taproot_witness(tx, 0, sig_result.signature);
    }

    return tx;
}

} // namespace taproot
} // namespace ailee
