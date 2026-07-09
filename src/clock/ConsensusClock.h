#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace ailee {
namespace clock {

struct ClockPacket {
    uint64_t epoch_id = 0;
    uint64_t consensus_tick = 0;
    std::array<uint8_t, 32> anchor_root = {0};

    std::vector<uint8_t> to_bytes() const;
    std::array<uint8_t, 32> hash() const;
};

std::array<uint8_t, 32> route_hash(const ClockPacket& packet);

class ConsensusClock {
public:
    ConsensusClock() = default;

    ClockPacket update(uint64_t new_epoch_id,
                       const std::array<uint8_t, 32>& new_anchor_root);

private:
    uint64_t current_epoch_id = 0;
    uint64_t consensus_tick = 0;
    std::array<uint8_t, 32> last_anchor_root = {0};
    ClockPacket last_packet;
};

} // namespace clock
} // namespace ailee
