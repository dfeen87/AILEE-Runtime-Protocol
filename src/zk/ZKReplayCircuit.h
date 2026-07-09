#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "../clock/ConsensusClock.h"

namespace ailee {
namespace zk {

struct ReplayProofArtifact {
    uint64_t epoch_id;
    uint64_t consensus_tick;
    std::array<uint8_t, 32> anchor_root;
    std::array<uint8_t, 32> packet_hash;
    std::array<uint8_t, 32> proof_hash;

    std::vector<uint8_t> to_bytes() const;
    std::array<uint8_t, 32> hash() const;
};

class ZKReplayCircuit {
public:
    ReplayProofArtifact generate(const clock::ClockPacket& packet) const;
};

} // namespace zk
} // namespace ailee
