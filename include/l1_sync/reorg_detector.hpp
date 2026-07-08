#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace ailee {
namespace l1_sync {

struct ReorgResolution {
    bool reorg_occurred;
    uint64_t rollback_height;
    std::vector<std::array<uint8_t, 32>> detached_blocks;
    std::vector<std::array<uint8_t, 32>> attached_blocks;
};

// Forward declaration since it will be implemented in reorg_detector.cpp
class ReorgDetector {
public:
    static ReorgResolution detect_and_resolve(
        const std::vector<std::array<uint8_t, 32>>& current_chain,
        const std::vector<std::array<uint8_t, 32>>& new_chain);
};

} // namespace l1_sync
} // namespace ailee
