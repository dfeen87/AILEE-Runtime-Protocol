#include "BitcoinAnchoringInterface.hpp"
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>
#include <vector>

namespace ailee {
namespace heartbeat {

static std::vector<uint8_t> serialize_anchor_payload(uint64_t epoch, const std::string& state_root) {
    std::vector<uint8_t> out;
    for (int i = 7; i >= 0; --i) {
        out.push_back((epoch >> (i * 8)) & 0xFF);
    }
    out.insert(out.end(), state_root.begin(), state_root.end());
    return out;
}

static std::string sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    std::stringstream ss;
    for (uint8_t byte : hash) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

static std::string build_anchor_metadata(uint64_t epoch) {
    std::stringstream meta;
    meta << "metadata_epoch_" << epoch;
    return meta.str();
}

AnchorResult DeterministicBitcoinAnchor::anchor_state_root(
    uint64_t epoch,
    const std::string& state_root
) {
    // 1. Canonical serialization
    std::vector<uint8_t> serialized = serialize_anchor_payload(epoch, state_root);

    // 2. Deterministic hashing (SHA-256 via secp256k1)
    std::string commitment_hash = sha256(serialized);

    // 3. Deterministic metadata
    std::string metadata = build_anchor_metadata(epoch);

    return AnchorResult{
        commitment_hash,
        metadata
    };
}

} // namespace heartbeat
} // namespace ailee
