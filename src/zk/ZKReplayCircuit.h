// SPDX-License-Identifier: MIT
// ZKReplayCircuit.h

#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "../clock/ConsensusClock.h"

namespace ailee {
namespace zk {

/**
 * @brief Deterministic data structure representing a replay proof artifact
 * that can later be verified by a zero-knowledge backend.
 */
struct ReplayProofArtifact {
    uint64_t epoch_id;
    uint64_t consensus_tick;
    std::array<uint8_t, 32> anchor_root;
    std::array<uint8_t, 32> packet_hash;
    std::array<uint8_t, 32> proof_hash; // SHA256 over serialized artifact (excluding proof_hash itself)

    /**
     * @brief Serializes the artifact to a stable byte sequence using little-endian encoding.
     * Includes epoch_id, consensus_tick, anchor_root, and packet_hash.
     */
    std::vector<uint8_t> to_bytes() const;

    /**
     * @brief Computes the SHA256 hash over the serialized bytes.
     */
    std::array<uint8_t, 32> hash() const;
};

/**
 * @brief Foundational module for generating deterministic replay-proof artifacts.
 * This class is entirely stateless and deterministic.
 */
class ZKReplayCircuit {
public:
    ZKReplayCircuit() = default;

    /**
     * @brief Consumes a ClockPacket and produces a ReplayProofArtifact.
     *
     * @param packet The ClockPacket from which to derive the artifact.
     * @return ReplayProofArtifact The deterministically generated artifact.
     */
    ReplayProofArtifact generate(const ailee::clock::ClockPacket& packet) const;
};

} // namespace zk
} // namespace ailee
