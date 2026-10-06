#pragma once

#include <cstdint>
#include <vector>

namespace ailee {
namespace l1_sync {

// Monotonic, jitter-neutral clock derived from L1 data.
struct BitcoinClockState {
    uint64_t height = 0;
    double consensus_time = 0.0;   // derived from MTP, smoothed
    double interval_seconds = 0.0; // moving estimate of block interval
};

} // namespace l1_sync
} // namespace ailee
