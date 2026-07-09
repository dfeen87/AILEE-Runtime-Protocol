#include "TaprootScript.h"
#include "taproot/TaprootUtils.h"
#include <openssl/sha.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <cstring>
#include <vector>

namespace ailee {
namespace taproot {

namespace {

bool tweak_taproot_output_key(const std::array<uint8_t, 32>& internal_key,
                              const std::array<uint8_t, 32>& tapleaf_hash,
                              std::array<uint8_t, 32>& out_taproot_output_key) {
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!ctx) return false;

    secp256k1_xonly_pubkey pubkey;
    if (secp256k1_xonly_pubkey_parse(ctx, &pubkey, internal_key.data()) != 1) {
        secp256k1_context_destroy(ctx);
        return false;
    }

    // BIP341 TapTweak: tweak = tagged_hash("TapTweak", P || merkle_root)
    // merkle_root in this case is the tapleaf_hash.
    unsigned char tag_hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>("TapTweak"), 8, tag_hash);

    std::vector<uint8_t> preimage;
    preimage.reserve(SHA256_DIGEST_LENGTH + SHA256_DIGEST_LENGTH + 32 + 32);
    preimage.insert(preimage.end(), tag_hash, tag_hash + SHA256_DIGEST_LENGTH);
    preimage.insert(preimage.end(), tag_hash, tag_hash + SHA256_DIGEST_LENGTH);
    preimage.insert(preimage.end(), internal_key.begin(), internal_key.end());
    preimage.insert(preimage.end(), tapleaf_hash.begin(), tapleaf_hash.end());

    unsigned char tweak[SHA256_DIGEST_LENGTH];
    SHA256(preimage.data(), preimage.size(), tweak);

    secp256k1_pubkey output_pubkey;
    if (secp256k1_xonly_pubkey_tweak_add(ctx, &output_pubkey, &pubkey, tweak) != 1) {
        secp256k1_context_destroy(ctx);
        return false;
    }

    secp256k1_xonly_pubkey out_xonly;
    if (secp256k1_xonly_pubkey_from_pubkey(ctx, &out_xonly, nullptr, &output_pubkey) != 1) {
        secp256k1_context_destroy(ctx);
        return false;
    }

    secp256k1_xonly_pubkey_serialize(ctx, out_taproot_output_key.data(), &out_xonly);

    secp256k1_context_destroy(ctx);
    return true;
}

namespace {

uint32_t polymod_step(uint32_t pre) {
    uint8_t b = pre >> 25;
    return ((pre & 0x1ffffff) << 5) ^
           (-((b >> 0) & 1) & 0x3b6a57b2UL) ^
           (-((b >> 1) & 1) & 0x26508e6dUL) ^
           (-((b >> 2) & 1) & 0x1ea119faUL) ^
           (-((b >> 3) & 1) & 0x3d4233ddUL) ^
           (-((b >> 4) & 1) & 0x2a1462b3UL);
}

std::vector<uint8_t> expand_hrp(const std::string& hrp) {
    std::vector<uint8_t> ret;
    ret.reserve(hrp.size() * 2 + 1);
    for (char c : hrp) {
        ret.push_back(c >> 5);
    }
    ret.push_back(0);
    for (char c : hrp) {
        ret.push_back(c & 31);
    }
    return ret;
}

std::vector<uint8_t> convert_bits(const std::vector<uint8_t>& in, int frombits, int tobits, bool pad) {
    int acc = 0;
    int bits = 0;
    std::vector<uint8_t> ret;
    int maxv = (1 << tobits) - 1;
    for (uint8_t value : in) {
        acc = (acc << frombits) | value;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            ret.push_back((acc >> bits) & maxv);
        }
    }
    if (pad && bits > 0) {
        ret.push_back((acc << (tobits - bits)) & maxv);
    }
    return ret;
}

} // namespace

} // namespace

void TapLeaf::build_script() {
    // Bitcoin Script opcodes
    const uint8_t OP_PUSH32 = 0x20;
    const uint8_t OP_DROP = 0x75;
    const uint8_t OP_TRUE = 0x51;

    // The script pattern is: OP_PUSH32 <anchor_root> OP_DROP OP_TRUE
    script.clear();
    script.reserve(1 + anchor_root.size() + 1 + 1);

    script.push_back(OP_PUSH32);
    script.insert(script.end(), anchor_root.begin(), anchor_root.end());
    script.push_back(OP_DROP);
    script.push_back(OP_TRUE);
}

std::array<uint8_t, 32> compute_tapleaf_hash(const TapLeaf& leaf) {
    std::array<uint8_t, 32> hash_out;

    // BIP342-style tagged hashing: SHA256(SHA256("TapLeaf") || SHA256("TapLeaf") || leaf_script)
    const char* tag = "TapLeaf";
    std::array<uint8_t, 32> tag_hash;
    SHA256(reinterpret_cast<const unsigned char*>(tag), std::strlen(tag), tag_hash.data());

    // Prepare buffer: tag_hash || tag_hash || leaf_script
    std::vector<uint8_t> buffer;
    buffer.reserve(32 + 32 + leaf.script.size());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), leaf.script.begin(), leaf.script.end());

    // Compute final hash
    SHA256(buffer.data(), buffer.size(), hash_out.data());

    return hash_out;
}

void TaprootOutput::compute_output_key(const std::array<uint8_t, 32>& tapleaf_hash) {
    if (!tweak_taproot_output_key(internal_key, tapleaf_hash, taproot_output_key)) {
        // Tweak failed, zero out the output key to indicate failure state (no exceptions)
        std::fill(taproot_output_key.begin(), taproot_output_key.end(), 0);
    }
}

std::string to_bech32m(const std::string& hrp, const std::vector<uint8_t>& witness_program) {
    const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    uint32_t BECH32M_CONST = 0x2bc830a3;

    std::vector<uint8_t> data;
    data.push_back(1); // witness version 1
    std::vector<uint8_t> conv = convert_bits(witness_program, 8, 5, true);
    data.insert(data.end(), conv.begin(), conv.end());

    std::vector<uint8_t> values = expand_hrp(hrp);
    values.insert(values.end(), data.begin(), data.end());

    // polymod checksum
    uint32_t chk = 1;
    for (uint8_t v : values) {
        chk = polymod_step(chk) ^ v;
    }
    for (int i = 0; i < 6; ++i) {
        chk = polymod_step(chk) ^ 0;
    }
    chk ^= BECH32M_CONST;

    std::vector<uint8_t> checksum;
    for (int i = 0; i < 6; ++i) {
        checksum.push_back((chk >> (5 * (5 - i))) & 31);
    }

    std::string ret = hrp + "1";
    for (uint8_t v : data) {
        ret += CHARSET[v];
    }
    for (uint8_t v : checksum) {
        ret += CHARSET[v];
    }
    return ret;
}

TaprootOutput build_taproot_for_anchor(const std::array<uint8_t, 32>& anchor_root,
                                       const std::array<uint8_t, 32>& internal_key) {
    TaprootOutput output;
    output.internal_key = internal_key;

    TapLeaf leaf;
    leaf.anchor_root = anchor_root;
    leaf.build_script();

    std::array<uint8_t, 32> tapleaf_hash = compute_tapleaf_hash(leaf);

    output.compute_output_key(tapleaf_hash);

    std::vector<uint8_t> wp(output.taproot_output_key.begin(), output.taproot_output_key.end());
    output.bech32m_address = to_bech32m("bc", wp);

    return output;
}

AnchorCommitTapLeaf TaprootScript::build_anchor_commit_tapleaf(
    const std::array<uint8_t, 32>& anchor_root,
    const std::array<uint8_t, 32>& metadata_hash
) {
    AnchorCommitTapLeaf result;

    const uint8_t OP_PUSH32 = 0x20;
    const uint8_t OP_DROP = 0x75;
    const uint8_t OP_TRUE = 0x51;

    // script length: 1 + 32 + 1 + 1 + 32 + 1 + 1 = 69 bytes
    result.script.reserve(69);

    result.script.push_back(OP_PUSH32);
    result.script.insert(result.script.end(), anchor_root.begin(), anchor_root.end());
    result.script.push_back(OP_DROP);
    result.script.push_back(OP_PUSH32);
    result.script.insert(result.script.end(), metadata_hash.begin(), metadata_hash.end());
    result.script.push_back(OP_DROP);
    result.script.push_back(OP_TRUE);

    // BIP342 TapLeaf hash
    const char* tag = "TapLeaf";
    std::array<uint8_t, 32> tag_hash;
    SHA256(reinterpret_cast<const unsigned char*>(tag), std::strlen(tag), tag_hash.data());

    std::vector<uint8_t> buffer;
    // reserve: 32 (tag) + 32 (tag) + 1 (version 0xC0) + varint (up to 9 bytes) + 69 (script)
    buffer.reserve(32 + 32 + 1 + 9 + 69);
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());

    // TapLeaf version 0xC0
    buffer.push_back(0xC0);

    // CompactSize script length
    write_compact_size(buffer, result.script.size());

    buffer.insert(buffer.end(), result.script.begin(), result.script.end());

    SHA256(buffer.data(), buffer.size(), result.leaf_hash.data());

    return result;
}

} // namespace taproot
} // namespace ailee
