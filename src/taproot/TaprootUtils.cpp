#include "taproot/TaprootUtils.h"

namespace ailee {
namespace taproot {

void write_uint32_le(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back(static_cast<uint8_t>((val >> 0) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

void write_uint64_le(std::vector<uint8_t>& out, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((val >> (8 * i)) & 0xFF));
    }
}

void write_compact_size(std::vector<uint8_t>& out, uint64_t val) {
    if (val < 0xFD) {
        out.push_back(static_cast<uint8_t>(val));
    } else if (val <= 0xFFFF) {
        out.push_back(0xFD);
        out.push_back(static_cast<uint8_t>((val >> 0) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    } else if (val <= 0xFFFFFFFF) {
        out.push_back(0xFE);
        out.push_back(static_cast<uint8_t>((val >> 0) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    } else {
        out.push_back(0xFF);
        write_uint64_le(out, val);
    }
}

} // namespace taproot
} // namespace ailee
