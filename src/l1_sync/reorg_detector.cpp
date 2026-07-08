#include "l1_sync/reorg_detector.hpp"
#include <algorithm>
#include <cstring>

namespace ailee {
namespace l1_sync {

ReorgResolution ReorgDetector::detect_and_resolve(
    const std::vector<std::array<uint8_t, 32>>& current_chain,
    const std::vector<std::array<uint8_t, 32>>& new_chain)
{
    ReorgResolution resolution;
    resolution.reorg_occurred = false;
    resolution.rollback_height = 0;

    size_t min_len = std::min(current_chain.size(), new_chain.size());
    size_t branch_idx = 0;

    // Find the branch point
    while (branch_idx < min_len && std::memcmp(current_chain[branch_idx].data(), new_chain[branch_idx].data(), 32) == 0) {
        branch_idx++;
    }

    if (branch_idx < current_chain.size()) {
        resolution.reorg_occurred = true;
        resolution.rollback_height = branch_idx; // relative height in this window

        for (size_t i = branch_idx; i < current_chain.size(); ++i) {
            resolution.detached_blocks.push_back(current_chain[i]);
        }
        for (size_t i = branch_idx; i < new_chain.size(); ++i) {
            resolution.attached_blocks.push_back(new_chain[i]);
        }
    }

    return resolution;
}

} // namespace l1_sync
} // namespace ailee
