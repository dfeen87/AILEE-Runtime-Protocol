#pragma once

#include <cstdint>
#include <vector>

namespace ailee {
namespace anchor {

struct AnchorMetadata {
    uint64_t epoch_number;
    uint32_t version;
    uint32_t replay_window;
    uint32_t anchor_type;
    uint32_t flags;

    AnchorMetadata(
        uint64_t epoch_number,
        uint32_t version,
        uint32_t replay_window,
        uint32_t anchor_type,
        uint32_t flags
    );

    std::vector<uint8_t> to_bytes() const;
};

} // namespace anchor
} // namespace ailee
