#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

namespace ailee {
    namespace l1 {
        class ReorgDetector;
    }
}

namespace ailee::l2 {

// Forward declaration
class Mempool;

/**
 * BlockProducer - Time-based block production for L2 chain
 * 
 * Produces blocks at a configurable interval (default: 1 second)
 * Tracks block height, transaction count, and anchor commitments
 * Pulls transactions from the mempool and includes them in blocks
 */
class BlockProducer {
public:
    struct Config {
        std::uint64_t blockIntervalMs = 1000;    // 1 block per second
        std::uint64_t commitmentInterval = 100;   // Anchor every 100 blocks
        std::uint64_t maxTransactionsPerBlock = 1000; // Max transactions per block
    };

    struct State {
        std::uint64_t blockHeight = 0;
        std::uint64_t totalTransactions = 0;
        std::uint64_t lastAnchorHeight = 0;
        std::uint64_t lastBlockTimestampMs = 0;
        std::uint64_t pendingTransactions = 0;
    };

    explicit BlockProducer(const Config& config);
    ~BlockProducer();

    // Start/stop block production
    void start();
    void stop();

    // Get current state (thread-safe)
    State getState() const;

    // Set the mempool reference (must be called before start)
    void setMempool(Mempool* mempool);

    // Set the reorg detector reference
    void setReorgDetector(ailee::l1::ReorgDetector* detector);

    // Called by other systems to report transactions (deprecated - use mempool directly)
    void recordTransaction();

    void broadcastLatestBlockToMainnet();

private:
    void blockProductionLoop();
    void produceBlock();
    void checkAnchorCommitment();

    Config config_;
    mutable std::mutex stateMutex_;
    State state_;
    
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> producerThread_;
    
    Mempool* mempool_{nullptr}; // Non-owning pointer to mempool
    ailee::l1::ReorgDetector* reorgDetector_{nullptr}; // Non-owning pointer

    std::set<std::string> rejectedTxs_; // Track deterministically rejected transactions
};

} // namespace ailee::l2
