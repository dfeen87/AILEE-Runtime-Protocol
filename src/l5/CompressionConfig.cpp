#include "l5/CompressionConfig.h"

namespace ailee {
namespace l5 {

CompressionConfig CompressionConfig::default_config() {
    CompressionConfig cfg;
    cfg.enable_delta = true;
    cfg.enable_rle = true;
    cfg.enable_stable_hash = true;
    cfg.max_compressed_bytes = 1 << 20; // 1 MB deterministic cap
    return cfg;
}

} // namespace l5
} // namespace ailee
