#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <cstring>
#include <stdexcept>
#include "ReorgDetector.h"

// Forward declarations
namespace rocksdb {
    class DB;
}

namespace ailee::l1 {

// ------------------------------------------------------------------
// 64-BYTE CACHE ALIGNED DATA STRUCTURES
// ------------------------------------------------------------------

/**
 * @brief Cache-aligned, zero-allocation anchor record.
 * Fits efficiently in CPU cache lines to prevent memory fragmentation and GC pauses.
 * Max size: 128 bytes (two 64-byte cache lines)
 */
struct alignas(64) CacheAlignedAnchor {
    uint8_t anchorHash[32];      // L1 Anchor Hash (e.g. Taproot commitment)
    uint8_t l2StateRoot[32];     // The committed L2 State Root
    uint8_t bitcoinTxId[32];     // L1 Settlement TxID

    uint64_t bitcoinHeight;      // The L1 block height
    uint64_t broadcastTime;      // Deterministic protocol timestamp
    uint32_t confirmations;      // Number of confirmed blocks
    uint16_t retryCount;         // Network retry counter
    uint8_t  status;             // Maps to AnchorStatus enum

    // Padding to ensure exactly 128 bytes (2 * 64)
    // Size breakdown: 32+32+32 (96) + 8+8+4+2+1 (23) = 119 bytes.
    // 128 - 119 = 9 bytes padding.
    uint8_t  _padding[9];

    // Helper: Convert to human readable strings
    std::string getAnchorHashStr() const;
    std::string getL2StateRootStr() const;
    std::string getBitcoinTxIdStr() const;

    // Helper: Convert from dynamic string
    void setAnchorHashStr(const std::string& hexStr);
    void setL2StateRootStr(const std::string& hexStr);
    void setBitcoinTxIdStr(const std::string& hexStr);
};
static_assert(sizeof(CacheAlignedAnchor) == 128, "CacheAlignedAnchor must be exactly 128 bytes (two 64-byte cache lines)");
static_assert(alignof(CacheAlignedAnchor) == 64, "CacheAlignedAnchor must be 64-byte aligned");

/**
 * @brief Cache-aligned Reorg Event.
 * Max size: 128 bytes.
 */
struct alignas(64) CacheAlignedReorgEvent {
    uint8_t oldBlockHash[32];
    uint8_t newBlockHash[32];
    uint64_t reorgHeight;
    uint64_t detectedAtTime;

    // Up to 1 invalidated anchor embedded directly.
    // If more, they are stored in a linked list or secondary index.
    uint8_t primaryInvalidatedAnchor[32];
    uint16_t totalInvalidatedAnchors;

    // Padding to ensure exactly 128 bytes
    // Size breakdown: 32+32 (64) + 8+8 (16) + 32 + 2 = 114 bytes
    // 128 - 114 = 14 bytes padding
    uint8_t _padding[14];
};
static_assert(sizeof(CacheAlignedReorgEvent) == 128, "CacheAlignedReorgEvent must be exactly 128 bytes");
static_assert(alignof(CacheAlignedReorgEvent) == 64, "CacheAlignedReorgEvent must be 64-byte aligned");

/**
 * @brief Deterministic, zero-allocation parsed settlement artifacts.
 * This is meant to be passed into pure functions for state root pipelines.
 */
struct alignas(64) SettlementIngestion {
    CacheAlignedAnchor latest_anchor; // 128 bytes
    uint64_t total_anchors_processed; // 8 bytes
    uint32_t latest_confirmations; // 4 bytes
    uint32_t reorgs_detected; // 4 bytes
    uint8_t _padding[48]; // 128 + 8 + 4 + 4 = 144 bytes, padding 48 makes it 192 bytes total
};
static_assert(sizeof(SettlementIngestion) == 192, "SettlementIngestion must be exactly 192 bytes");
static_assert(alignof(SettlementIngestion) == 64, "SettlementIngestion must be 64-byte aligned");


// ------------------------------------------------------------------
// SETTLEMENT INGESTION ENGINE
// ------------------------------------------------------------------

/**
 * @brief Passive, zero-copy read engine for L1 settlement state.
 * Interfaces with the same RocksDB used by ReorgDetector but never blocks or allocates
 * strings on the critical ingestion path.
 */
class SettlementIngestionEngine {
public:
    explicit SettlementIngestionEngine(std::shared_ptr<rocksdb::DB> db);
    ~SettlementIngestionEngine() = default;

    // Fast, zero-copy read from RocksDB
    std::optional<CacheAlignedAnchor> getAnchorZeroCopy(const std::string& anchorHashHex) const;

    // Ingests confirmed blocks passively
    std::vector<CacheAlignedAnchor> ingestConfirmedAnchors(uint64_t currentL1Height, uint64_t confirmationThreshold) const;

    // Deterministic state rollback handler
    std::vector<CacheAlignedAnchor> ingestReorgInvalidations(uint64_t reorgHeight) const;

    // Write a cache aligned anchor directly (bypassing std::string)
    bool writeAnchorZeroCopy(const CacheAlignedAnchor& anchor, std::string* err = nullptr);

private:
    std::shared_ptr<rocksdb::DB> db_;
};

// Utilities for hex string conversions without allocating in loops
namespace utils {
    void hexToBytes(const std::string& hex, uint8_t* out, size_t outLen);
    std::string bytesToHex(const uint8_t* bytes, size_t len);
}

} // namespace ailee::l1
