#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace ailee {
namespace anchor {

struct AnchorRecord {
    uint64_t epoch_id;
    std::array<uint8_t, 32> state_root;
    std::array<uint8_t, 32> replay_root;
    uint32_t version;
    uint32_t network_id;

    std::vector<uint8_t> to_bytes() const;
    std::array<uint8_t, 32> anchor_root() const;
};

} // namespace anchor
} // namespace ailee
