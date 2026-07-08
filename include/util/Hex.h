#pragma once

#include <string>
#include <array>
#include <cstdint>

namespace ailee {
namespace util {

inline std::string to_hex(const std::array<uint8_t, 32>& bytes) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (uint8_t b : bytes) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0F]);
    }
    return out;
}

} // namespace util
} // namespace ailee
