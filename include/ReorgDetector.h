#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <memory>

// Forward declaration to avoid including RocksDB header
namespace rocksdb {
    class DB;
    class ColumnFamilyHandle;
}

namespace ailee::l1 {

enum class AnchorStatus {
    PENDING = 0,          // Broadcast but not yet confirmed
    CONFIRMED = 1,        // Confirmed in blockchain
    INVALIDATED_REORG = 2,// Invalidated due to blockchain reorg
    FAILED_ORPHANED = 3   // Failed to confirm (stuck/rejected)
};

struct AnchorCommitmentRecord {
    std::string anchorHash;
    std::string bitcoinTxId;
    std::uint64_t bitcoinHeight{0};
    std::uint64_t confirmations{0};
    std::uint64_t broadcastTime{0};
    std::uint32_t retryCount{0};
    AnchorStatus status{AnchorStatus::PENDING};
    std::string l2StateRoot;
};

struct ReorgEvent {
    std::uint64_t reorgHeight;
    std::string oldBlockHash;
    std::string newBlockHash;
    std::uint64_t detectedAtTime{0};
    std::vector<std::string> invalidatedAnchors;
};

class ReorgDetector {
public:
    // Constructor now requires a database path for persistent storage
    ReorgDetector(const std::string& dbPath,
                  std::uint64_t confirmationThreshold = 6,
                  std::uint64_t maxAnchorPendingTime = 3600000); // 1 hour in ms

    ~ReorgDetector();

    // Initialize the database (must be called after construction)
    bool initialize(std::string* err = nullptr);

    // Close the database
    void close();

    // Track a new block at a given height (persisted to disk)
    bool trackBlock(std::uint64_t height, const std::string& blockHash, 
                   std::uint64_t timestamp);

    // Detect if a reorg occurred at the given height
    std::optional<ReorgEvent> detectReorg(std::uint64_t height, 
                                         const std::string& newBlockHash,
                                         std::uint64_t timestamp);

    // Register an anchor commitment (persisted to disk)
    bool registerAnchor(const AnchorCommitmentRecord& anchor, std::string* err = nullptr);

    // Update anchor confirmation count (persisted to disk)
    bool updateAnchorConfirmations(const std::string& anchorHash, 
                                  std::uint64_t confirmations,
                                  std::string* err = nullptr);

    // Handle a detected reorg - returns list of invalidated anchor hashes (persisted)
    std::vector<std::string> handleReorg(std::uint64_t reorgHeight);

    // Get anchors that are orphaned (pending too long)
    std::vector<AnchorCommitmentRecord> getOrphanedAnchors(std::uint64_t currentTime) const;

    // Get current status of an anchor (loaded from disk)
    std::optional<AnchorCommitmentRecord> getAnchorStatus(const std::string& anchorHash) const;

    // Update anchor status (persisted to disk)
    bool updateAnchorStatus(const std::string& anchorHash, AnchorStatus newStatus,
                           std::string* err = nullptr);

    // Get all reorg events (loaded from disk)
    std::vector<ReorgEvent> getReorgHistory() const;

    // Get recent reorg events (limit to N most recent)
    std::vector<ReorgEvent> getRecentReorgHistory(std::size_t maxEvents = 100) const;

    // Set callback for reorg detection
    void setReorgCallback(std::function<void(const ReorgEvent&)> callback) {
        reorgCallback_ = std::move(callback);
    }

    // Clear old block tracking data (keep last N blocks) - persistent pruning
    bool pruneOldBlocks(std::uint64_t keepLastN = 1000, std::string* err = nullptr);

    // Check if system should halt due to deep reorg
    bool shouldHaltForDeepReorg(std::uint64_t reorgDepth) const {
        return reorgDepth > confirmationThreshold_;
    }

    // Get block hash at a specific height (loaded from disk)
    std::optional<std::string> getBlockHashAtHeight(std::uint64_t height) const;

    // Get all anchors with a specific status
    std::vector<AnchorCommitmentRecord> getAnchorsByStatus(AnchorStatus status) const;

    // --- Ambient AI Reputation CF API ---
    bool setReputation(const std::string& peerId, const std::string& repData, std::string* err = nullptr);
    std::optional<std::string> getReputation(const std::string& peerId) const;

private:
    std::uint64_t confirmationThreshold_;
    std::uint64_t maxAnchorPendingTime_;
    std::string dbPath_;
    
    // RocksDB instance for persistent storage
    std::unique_ptr<rocksdb::DB> db_;
    rocksdb::ColumnFamilyHandle* defaultCf_ = nullptr;
    rocksdb::ColumnFamilyHandle* reputationCf_ = nullptr;
    
    // Optional callback for reorg events
    std::function<void(const ReorgEvent&)> reorgCallback_;

public:
    // Persistent storage keys
    static constexpr const char* kBlockHashPrefix = "block:";
    static constexpr const char* kAnchorPrefix = "anchor:";
    static constexpr const char* kReorgEventPrefix = "reorg:";
    static constexpr const char* kReorgCounterKey = "reorg_counter";

private:
    // Helper methods for serialization/deserialization
    std::string serializeAnchor(const AnchorCommitmentRecord& anchor) const;
    std::optional<AnchorCommitmentRecord> deserializeAnchor(const std::string& data) const;
    std::string serializeReorgEvent(const ReorgEvent& event) const;
    std::optional<ReorgEvent> deserializeReorgEvent(const std::string& data) const;

    // Helper to store reorg event
    bool storeReorgEvent(const ReorgEvent& event, std::string* err = nullptr);
};

} // namespace ailee::l1
