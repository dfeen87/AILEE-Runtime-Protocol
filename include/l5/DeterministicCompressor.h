#pragma once

#include "l5/CompressionConfig.h"
#include "l4/ReplayTick.h"
#include <vector>
#include <cstdint>

namespace ailee {
namespace l5 {

class DeterministicCompressor {
public:
    DeterministicCompressor(const CompressionConfig& cfg = CompressionConfig::default_config());

    // Compress a federation tick deterministically
    std::vector<uint8_t> compress_tick(
        const l4::ClusterView& view,
        const l4::ReplayTick& tick
    );

    // Decompress deterministically (for replay)
    l4::ReplayTick decompress_tick(
        const std::vector<uint8_t>& data
    );

private:
    CompressionConfig config;
    std::vector<uint8_t> previous_serialized;
    bool has_previous;

    // Deterministic helpers
    std::vector<uint8_t> delta_encode(const std::vector<uint8_t>& previous, const std::vector<uint8_t>& current);
    std::vector<uint8_t> rle_encode(const std::vector<uint8_t>& raw);
    std::vector<uint8_t> stable_hash(const std::vector<uint8_t>& raw);

    // Reverse operations
    std::vector<uint8_t> delta_decode(const std::vector<uint8_t>& previous, const std::vector<uint8_t>& delta);
    std::vector<uint8_t> rle_decode(const std::vector<uint8_t>& raw);
};

} // namespace l5
} // namespace ailee
