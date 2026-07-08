#pragma once

#include <cstdint>
#include <vector>

namespace ailee {
namespace l1_sync {

// Monotonic, jitter-neutral clock derived from L1 data.
struct BitcoinClockState {
    uint64_t height;
    double consensus_time;   // derived from MTP, smoothed
    double interval_seconds; // moving estimate of block interval
};

} // namespace l1_sync
} // namespace ailee
