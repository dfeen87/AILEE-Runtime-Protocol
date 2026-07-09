// SPDX-License-Identifier: MIT
// ConsensusClock.cpp

#include "ConsensusClock.h"
#include <openssl/sha.h>

namespace ailee {
namespace clock {

std::vector<uint8_t> ClockPacket::to_bytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(sizeof(epoch_id) + sizeof(consensus_tick) + anchor_root.size());

    // Explicit little-endian encoding for epoch_id
    for (size_t i = 0; i < sizeof(epoch_id); ++i) {
        bytes.push_back(static_cast<uint8_t>((epoch_id >> (i * 8)) & 0xFF));
    }

    // Explicit little-endian encoding for consensus_tick
    for (size_t i = 0; i < sizeof(consensus_tick); ++i) {
        bytes.push_back(static_cast<uint8_t>((consensus_tick >> (i * 8)) & 0xFF));
    }

    // Append anchor_root bytes
    bytes.insert(bytes.end(), anchor_root.begin(), anchor_root.end());

    return bytes;
}

std::array<uint8_t, 32> ClockPacket::hash() const {
    std::vector<uint8_t> bytes = to_bytes();
    std::array<uint8_t, 32> hash_out;

    SHA256(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), hash_out.data());

    return hash_out;
}

ClockPacket ConsensusClock::update(uint64_t new_epoch_id, const std::array<uint8_t, 32>& new_anchor_root) {
    if (new_epoch_id > epoch_id) {
        epoch_id = new_epoch_id;
        anchor_root = new_anchor_root;
        consensus_tick++;
    }

    return ClockPacket{epoch_id, consensus_tick, anchor_root};
}

std::array<uint8_t, 32> route_hash(const ClockPacket& packet) {
    return packet.hash();
}

} // namespace clock
} // namespace ailee
