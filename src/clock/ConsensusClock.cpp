#include "ConsensusClock.h"
#include <openssl/sha.h>

namespace ailee {
namespace clock {

std::vector<uint8_t> ClockPacket::to_bytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(8 + 8 + 32);

    // epoch_id (uint64_t, little-endian)
    for (int i = 0; i < 8; ++i) {
        bytes.push_back(static_cast<uint8_t>((epoch_id >> (8 * i)) & 0xFF));
    }

    // consensus_tick (uint64_t, little-endian)
    for (int i = 0; i < 8; ++i) {
        bytes.push_back(static_cast<uint8_t>((consensus_tick >> (8 * i)) & 0xFF));
    }

    // anchor_root (32 bytes)
    for (uint8_t byte : anchor_root) {
        bytes.push_back(byte);
    }

    return bytes;
}

std::array<uint8_t, 32> ClockPacket::hash() const {
    std::vector<uint8_t> serialized = to_bytes();
    std::array<uint8_t, 32> h;
    SHA256(serialized.data(), serialized.size(), h.data());
    return h;
}

std::array<uint8_t, 32> route_hash(const ClockPacket& packet) {
    return packet.hash();
}

ClockPacket ConsensusClock::update(uint64_t new_epoch_id,
                                   const std::array<uint8_t, 32>& new_anchor_root) {
    if (new_epoch_id > current_epoch_id) {
        current_epoch_id = new_epoch_id;
        consensus_tick += 1;
        last_anchor_root = new_anchor_root;

        last_packet.epoch_id = current_epoch_id;
        last_packet.consensus_tick = consensus_tick;
        last_packet.anchor_root = last_anchor_root;
    }

    return last_packet;
}

} // namespace clock
} // namespace ailee
