#include "AnchorRecord.h"
#include <openssl/sha.h>

namespace ailee {
namespace anchor {

std::vector<uint8_t> AnchorRecord::to_bytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(8 + 32 + 32 + 4 + 4);

    // epoch_id (uint64_t, little-endian)
    for (int i = 0; i < 8; ++i) {
        bytes.push_back(static_cast<uint8_t>((epoch_id >> (8 * i)) & 0xFF));
    }

    // state_root (32 bytes)
    for (uint8_t byte : state_root) {
        bytes.push_back(byte);
    }

    // replay_root (32 bytes)
    for (uint8_t byte : replay_root) {
        bytes.push_back(byte);
    }

    // version (uint32_t, little-endian)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((version >> (8 * i)) & 0xFF));
    }

    // network_id (uint32_t, little-endian)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((network_id >> (8 * i)) & 0xFF));
    }

    return bytes;
}

std::array<uint8_t, 32> AnchorRecord::anchor_root() const {
    std::vector<uint8_t> serialized = to_bytes();
    std::array<uint8_t, 32> hash;
    SHA256(serialized.data(), serialized.size(), hash.data());
    return hash;
}

} // namespace anchor
} // namespace ailee
