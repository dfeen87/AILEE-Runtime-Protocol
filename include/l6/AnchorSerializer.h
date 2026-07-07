#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace ailee {
namespace l6 {

struct AnchorRecord {
    uint8_t version;
    uint64_t replay_height;
    uint64_t coherence_score;
    std::array<uint8_t, 32> state_root;
};

std::vector<uint8_t> serialize(const AnchorRecord& rec);
bool deserialize(const std::vector<uint8_t>& bytes, AnchorRecord& out);

} // namespace l6
} // namespace ailee
