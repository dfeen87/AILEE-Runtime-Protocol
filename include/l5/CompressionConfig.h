#pragma once

#include <cstddef>

namespace ailee {
namespace l5 {

struct CompressionConfig {
    bool enable_delta;
    bool enable_rle;
    bool enable_stable_hash;

    // Maximum allowed compressed size (for dashboard safety)
    size_t max_compressed_bytes;

    static CompressionConfig default_config();
};

} // namespace l5
} // namespace ailee
