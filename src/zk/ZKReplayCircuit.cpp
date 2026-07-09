#include "ZKReplayCircuit.h"
#include <openssl/sha.h>

namespace ailee {
namespace zk {

std::vector<uint8_t> ReplayProofArtifact::to_bytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(8 + 8 + 32 + 32);

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

    // packet_hash (32 bytes)
    for (uint8_t byte : packet_hash) {
        bytes.push_back(byte);
    }

    // Note: proof_hash MUST NOT be included to avoid circular dependency
    return bytes;
}

std::array<uint8_t, 32> ReplayProofArtifact::hash() const {
    std::vector<uint8_t> serialized = to_bytes();
    std::array<uint8_t, 32> h;
    SHA256(serialized.data(), serialized.size(), h.data());
    return h;
}

ReplayProofArtifact ZKReplayCircuit::generate(const clock::ClockPacket& packet) const {
    ReplayProofArtifact artifact;
    artifact.epoch_id = packet.epoch_id;
    artifact.consensus_tick = packet.consensus_tick;
    artifact.anchor_root = packet.anchor_root;
    artifact.packet_hash = packet.hash();
    artifact.proof_hash = artifact.hash();
    return artifact;
}

} // namespace zk
} // namespace ailee
