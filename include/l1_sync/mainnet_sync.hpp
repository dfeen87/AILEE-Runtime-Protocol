#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <cstddef>
#include <deque>
#include <string>
#include "l1_sync/sync_events.hpp"
#include "l1_sync/bitcoin_clock.hpp"

namespace ailee {
namespace l1_sync {

// Types for ingestion
struct BlockHeader {
    int32_t version;
    std::array<uint8_t, 32> prev_hash;
    std::array<uint8_t, 32> merkle_root;
    uint32_t timestamp;
    uint32_t nBits;
    uint32_t nonce;
    std::array<uint8_t, 32> hash; // computed or provided
    uint64_t height;
};

using HeaderBatch = std::vector<BlockHeader>;

struct MempoolDelta {
    bool is_add;
    std::array<uint8_t, 32> txid;
    uint64_t fee;
    uint32_t size;
};

using MempoolDeltaBatch = std::vector<MempoolDelta>;

struct MempoolEntry {
    std::array<uint8_t, 32> txid;
    uint64_t fee;
    uint32_t size;

    // Sort logic
    bool operator<(const MempoolEntry& other) const;
};

using MempoolSnapshot = std::vector<MempoolEntry>;

class MainnetSyncManager {
public:
    MainnetSyncManager(size_t max_buffer_size = 144);

    // Ingest data and produce deterministic sync events
    void ingest_headers(const HeaderBatch& headers);
    void ingest_mempool_deltas(const MempoolDeltaBatch& deltas);

    SyncEventBatch drain_sync_events();

    const BitcoinClockState& get_clock() const { return clock; }
    const MempoolSnapshot& get_mempool() const { return mempool; }

    // Hardening V27 Metrics
    std::string compute_utxo_reflection_hash() const;
    double get_delta_auc() const { return delta_auc; }
    double get_spectral_drift() const { return spectral_drift; }
    void simulate_sync_cycle_metrics();

private:
    size_t max_buffer_size;
    std::deque<BlockHeader> header_buffer;
    MempoolSnapshot mempool;
    BitcoinClockState clock;
    std::vector<SyncEvent> pending_events;

    double delta_auc = 0.0;
    double spectral_drift = 0.0;
    std::vector<double> historical_intervals;

    void update_clock();
    void sort_mempool();
};

} // namespace l1_sync
} // namespace ailee
