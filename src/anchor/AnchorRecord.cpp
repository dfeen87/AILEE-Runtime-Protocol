#include "AnchorRecord.h"
#include <openssl/sha.h>

namespace ailee {

std::vector<uint8_t> AnchorRecord::to_bytes() const {
    std::vector<uint8_t> bytes;
    // 8 bytes for epoch_id + 32 for state_root + 32 for replay_root + 4 for version + 4 for network_id = 80 bytes
    bytes.reserve(80);

    // Little-endian serialization for epoch_id (uint64_t)
    for (int i = 0; i < 8; ++i) {
        bytes.push_back(static_cast<uint8_t>((epoch_id >> (8 * i)) & 0xFF));
    }

    // state_root (32 bytes)
    bytes.insert(bytes.end(), state_root.begin(), state_root.end());

    // replay_root (32 bytes)
    bytes.insert(bytes.end(), replay_root.begin(), replay_root.end());

    // Little-endian serialization for version (uint32_t)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((version >> (8 * i)) & 0xFF));
    }

    // Little-endian serialization for network_id (uint32_t)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((network_id >> (8 * i)) & 0xFF));
    }

    return bytes;
}

std::array<uint8_t, 32> AnchorRecord::anchor_root() const {
    std::vector<uint8_t> bytes = to_bytes();
    std::array<uint8_t, 32> root;

    SHA256(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), root.data());

    return root;
}

} // namespace ailee
