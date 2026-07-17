#include "anchor/AnchorMetadata.h"

namespace ailee {
namespace anchor {

AnchorMetadata::AnchorMetadata(
    uint64_t epoch_number_val,
    uint32_t version_val,
    uint32_t replay_window_val,
    uint32_t anchor_type_val,
    uint32_t flags_val
) : epoch_number(epoch_number_val),
    version(version_val),
    replay_window(replay_window_val),
    anchor_type(anchor_type_val),
    flags(flags_val) {}

std::vector<uint8_t> AnchorMetadata::to_bytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(8 + 4 + 4 + 4 + 4); // 24 bytes total

    // epoch_number (uint64_t, little-endian)
    for (int i = 0; i < 8; ++i) {
        bytes.push_back(static_cast<uint8_t>((epoch_number >> (8 * i)) & 0xFF));
    }

    // version (uint32_t, little-endian)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((version >> (8 * i)) & 0xFF));
    }

    // replay_window (uint32_t, little-endian)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((replay_window >> (8 * i)) & 0xFF));
    }

    // anchor_type (uint32_t, little-endian)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((anchor_type >> (8 * i)) & 0xFF));
    }

    // flags (uint32_t, little-endian)
    for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<uint8_t>((flags >> (8 * i)) & 0xFF));
    }

    return bytes;
}

} // namespace anchor
} // namespace ailee
