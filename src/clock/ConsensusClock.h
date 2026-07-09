// SPDX-License-Identifier: MIT
// ConsensusClock.h

#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace ailee {
namespace clock {

/**
 * @brief Deterministic packet representing the current consensus time.
 */
struct ClockPacket {
    uint64_t epoch_id;
    uint64_t consensus_tick;
    std::array<uint8_t, 32> anchor_root;

    /**
     * @brief Serializes the packet to a stable byte sequence using little-endian encoding.
     */
    std::vector<uint8_t> to_bytes() const;

    /**
     * @brief Computes the SHA256 hash over the serialized bytes.
     */
    std::array<uint8_t, 32> hash() const;
};

/**
 * @brief Deterministic clock for generating consensus time signals.
 */
class ConsensusClock {
public:
    ConsensusClock() = default;

    /**
     * @brief Updates the consensus clock with a new anchor root.
     *
     * Ensures strict monotonicity of epoch_id (new_epoch_id > current_epoch_id).
     * If the condition is violated, the update is ignored and the last valid state is returned.
     * Real epochs must start at 1 or greater.
     */
    ClockPacket update(uint64_t new_epoch_id, const std::array<uint8_t, 32>& new_anchor_root);

private:
    uint64_t epoch_id{0};
    uint64_t consensus_tick{0};
    std::array<uint8_t, 32> anchor_root{0};
};

/**
 * @brief Produces a deterministic route hash for a ClockPacket.
 */
std::array<uint8_t, 32> route_hash(const ClockPacket& packet);

} // namespace clock
} // namespace ailee
