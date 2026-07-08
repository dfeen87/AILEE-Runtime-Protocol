#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include "l1_sync/sync_events.hpp"
#include "l1_sync/bitcoin_clock.hpp"

namespace ailee {
namespace l1_sync {

using TxId = std::array<uint8_t, 32>;

enum class ReplayEventType : uint8_t {
    HeaderApplied = 0,
    ReorgRollback = 1,
    MempoolUpdate = 2,
    PhaseShift = 3
};

struct ReplayEvent {
    ReplayEventType type;
    uint64_t height;
    std::array<uint8_t, 32> block_hash;
    TxId txid;
};

struct ReplayInput {
    SyncEventBatch events;
    BitcoinClockState clock;
};

struct ReplayState {
    uint64_t current_height;
};

} // namespace l1_sync
} // namespace ailee
