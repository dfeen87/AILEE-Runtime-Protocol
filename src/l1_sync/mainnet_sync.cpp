#include "l1_sync/mainnet_sync.hpp"
#include <algorithm>
#include "l1_sync/reorg_detector.hpp"
#include <cstring>
#include <stdexcept>

namespace ailee {
namespace l1_sync {

bool MempoolEntry::operator<(const MempoolEntry& other) const {
    // Deterministic sort: txid -> fee -> size
    int cmp = std::memcmp(txid.data(), other.txid.data(), txid.size());
    if (cmp != 0) return cmp < 0;
    if (fee != other.fee) return fee < other.fee;
    return size < other.size;
}

MainnetSyncManager::MainnetSyncManager(size_t max_buffer_size)
    : max_buffer_size(max_buffer_size)
{
    clock.height = 0;
    clock.consensus_time = 0.0;
    clock.interval_seconds = 600.0; // default 10 mins
}

void MainnetSyncManager::ingest_headers(const HeaderBatch& headers) {
    for (const auto& header : headers) {
        // Validation: Prevhash check for Reorg detection
        if (!header_buffer.empty()) {
            const auto& tip = header_buffer.back();
            if (std::memcmp(tip.hash.data(), header.prev_hash.data(), 32) != 0 || header.height <= tip.height) {
                // Reorg detected due to prevhash mismatch or rollback

                // Use the ReorgDetector
                std::vector<std::array<uint8_t, 32>> current_chain;
                for (const auto& h : header_buffer) {
                    current_chain.push_back(h.hash);
                }
                std::vector<std::array<uint8_t, 32>> new_chain;
                new_chain.push_back(header.hash);

                ReorgResolution resolution = ReorgDetector::detect_and_resolve(current_chain, new_chain);

                if (resolution.reorg_occurred) {
                    SyncEvent event;
                    event.type = SyncEventType::ReorgDetected;
                    event.height = tip.height; // Emit at the height where the reorg was observed
                    event.reorg_target_height = resolution.rollback_height;
                    event.block_hash = header.hash;
                    pending_events.push_back(event);

                    // Truncate buffer to branch point
                    while (header_buffer.size() > resolution.rollback_height) {
                        header_buffer.pop_back();
                    }
                } else {
                    // Fallback to simple pop
                    while (!header_buffer.empty() && (header_buffer.back().height >= header.height ||
                           (header_buffer.size() > 0 && std::memcmp(header_buffer.back().hash.data(), header.prev_hash.data(), 32) != 0))) {
                        header_buffer.pop_back();
                    }
                }
            }
        }

        // Basic PoW Validation checks could go here (nonce, nBits, version)
        if (header.version < 1) {
            continue; // Skip obviously bad headers to maintain deterministic integrity
        }

        header_buffer.push_back(header);
        if (header_buffer.size() > max_buffer_size) {
            header_buffer.pop_front();
        }

        SyncEvent event;
        event.type = SyncEventType::HeaderApplied;
        event.height = header.height;
        event.block_hash = header.hash;
        pending_events.push_back(event);
    }

    update_clock();
    simulate_sync_cycle_metrics();
}

void MainnetSyncManager::ingest_mempool_deltas(const MempoolDeltaBatch& deltas) {
    bool changed = false;

    uint64_t current_height = clock.height;
    std::array<uint8_t, 32> current_hash = {0};
    if (!header_buffer.empty()) {
        current_hash = header_buffer.back().hash;
    }

    for (const auto& delta : deltas) {
        if (delta.is_add) {
            MempoolEntry entry;
            entry.txid = delta.txid;
            entry.fee = delta.fee;
            entry.size = delta.size;
            mempool.push_back(entry);

            SyncEvent event;
            std::memset(&event, 0, sizeof(event));
            event.type = SyncEventType::MempoolDeltaApplied;
            event.height = current_height;
            event.block_hash = current_hash;
            event.txid = delta.txid;
            pending_events.push_back(event);
            changed = true;
        } else {
            auto it = std::remove_if(mempool.begin(), mempool.end(),
                [&delta](const MempoolEntry& e) {
                    return std::memcmp(e.txid.data(), delta.txid.data(), e.txid.size()) == 0;
                });
            if (it != mempool.end()) {
                mempool.erase(it, mempool.end());

                SyncEvent event;
                std::memset(&event, 0, sizeof(event));
                event.type = SyncEventType::MempoolDeltaApplied;
                event.height = current_height;
                event.block_hash = current_hash;
                event.txid = delta.txid;
                pending_events.push_back(event);
                changed = true;
            }
        }
    }

    if (changed) {
        sort_mempool();
    }
}

SyncEventBatch MainnetSyncManager::drain_sync_events() {
    SyncEventBatch batch = std::move(pending_events);
    pending_events.clear();
    return batch;
}

void MainnetSyncManager::update_clock() {
    if (header_buffer.empty()) return;

    clock.height = header_buffer.back().height;

    // MTP - median time past over last 11 blocks
    std::vector<uint32_t> times;
    auto it = header_buffer.rbegin();
    for (int i = 0; i < 11 && it != header_buffer.rend(); ++i, ++it) {
        times.push_back(it->timestamp);
    }

    if (!times.empty()) {
        std::sort(times.begin(), times.end());
        clock.consensus_time = static_cast<double>(times[times.size() / 2]);
    }
}

void MainnetSyncManager::sort_mempool() {
    std::sort(mempool.begin(), mempool.end());
}




std::string ailee::l1_sync::MainnetSyncManager::compute_utxo_reflection_hash() const {
    uint64_t sum = clock.height;
    for (const auto& entry : mempool) {
        sum += entry.fee;
        sum += entry.size;
        for (uint8_t b : entry.txid) {
            sum += b;
        }
    }
    char hex_chars[] = "0123456789abcdef";
    std::string hash = "";
    for (int i = 0; i < 8; ++i) {
        hash += hex_chars[(sum >> ((7 - i) * 4)) & 0x0F];
    }
    return hash;
}

void ailee::l1_sync::MainnetSyncManager::simulate_sync_cycle_metrics() {
    double new_auc = mempool.size() > 0 ? 0.85 + (mempool.size() * 0.001) : 0.80;
    static double last_auc = 0.80;
    delta_auc = new_auc - last_auc;
    last_auc = new_auc;

    if (historical_intervals.size() >= 5) {
        double mean = 0;
        for (double val : historical_intervals) mean += val;
        mean /= historical_intervals.size();

        double variance = 0;
        for (double val : historical_intervals) variance += (val - mean) * (val - mean);

        spectral_drift = (variance / historical_intervals.size()) * 0.0001;
    } else {
        spectral_drift = 0.0;
    }
}
} // namespace l1_sync
} // namespace ailee
