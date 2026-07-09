#pragma once

#include <vector>
#include <cstdint>

namespace ailee {
namespace taproot {

// Helper to write a 32-bit integer in little-endian
void write_uint32_le(std::vector<uint8_t>& out, uint32_t val);

// Helper to write a 64-bit integer in little-endian
void write_uint64_le(std::vector<uint8_t>& out, uint64_t val);

// Helper to write a Bitcoin CompactSize (varint)
void write_compact_size(std::vector<uint8_t>& out, uint64_t val);

} // namespace taproot
} // namespace ailee
