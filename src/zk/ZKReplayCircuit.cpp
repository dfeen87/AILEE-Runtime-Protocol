// SPDX-License-Identifier: MIT
// ZKReplayCircuit.cpp

#include "ZKReplayCircuit.h"
#include <openssl/sha.h>

namespace ailee {
namespace zk {

std::vector<uint8_t> ReplayProofArtifact::to_bytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(sizeof(epoch_id) + sizeof(consensus_tick) + anchor_root.size() + packet_hash.size());

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

    // Append packet_hash bytes
    bytes.insert(bytes.end(), packet_hash.begin(), packet_hash.end());

    return bytes;
}

std::array<uint8_t, 32> ReplayProofArtifact::hash() const {
    std::vector<uint8_t> bytes = to_bytes();
    std::array<uint8_t, 32> hash_out;

    SHA256(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), hash_out.data());

    return hash_out;
}

ReplayProofArtifact ZKReplayCircuit::generate(const ailee::clock::ClockPacket& packet) const {
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
