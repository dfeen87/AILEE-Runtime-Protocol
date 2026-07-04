#include "ReorgDetector.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/iterator.h>
#include <rocksdb/write_batch.h>

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring> 
#include <optional>

namespace ailee::l1 {

namespace {

const std::size_t prefixLen = std::strlen(ReorgDetector::kBlockHashPrefix);

// Helper to create a key for block hash storage
[[maybe_unused]] std::string makeBlockKey(std::uint64_t height) {
    std::ostringstream oss;
    oss << ReorgDetector::kBlockHashPrefix 
        << std::setw(20) << std::setfill('0') << height;
    return oss.str();
}

// Helper to create a key for anchor storage
[[maybe_unused]] std::string makeAnchorKey(const std::string& anchorHash) {
    return std::string(ReorgDetector::kAnchorPrefix) + anchorHash;
}

// Helper to create a key for reorg event storage
std::string makeReorgEventKey(std::uint64_t eventId) {
    std::ostringstream oss;
    oss << ReorgDetector::kReorgEventPrefix 
        << std::setw(20) << std::setfill('0') << eventId;
    return oss.str();
}

// Parse height from block key
[[maybe_unused]] std::optional<std::uint64_t> parseHeightFromKey(const std::string& key) {
    if (key.size() <= prefixLen) {
        return std::nullopt;
    }
    try {
        return std::stoull(key.substr(prefixLen));
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace (anonymous)

ReorgDetector::ReorgDetector(const std::string& dbPath,
                             std::uint64_t confirmationThreshold,
                             std::uint64_t maxAnchorPendingTime)
    : confirmationThreshold_(confirmationThreshold),
      maxAnchorPendingTime_(maxAnchorPendingTime),
      dbPath_(dbPath) {
}

ReorgDetector::~ReorgDetector() {
    close();
}

bool ReorgDetector::initialize(std::string* err) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = false;
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
    column_families.push_back(rocksdb::ColumnFamilyDescriptor("reputation_cf", rocksdb::ColumnFamilyOptions()));
    rocksdb::DB* dbPtr = nullptr;
    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::Status status = rocksdb::DB::Open(options, dbPath_, column_families, &handles, &dbPtr);
    if (!status.ok()) {
        rocksdb::Options create_opts = options;
        status = rocksdb::DB::Open(create_opts, dbPath_, &dbPtr);
        if (status.ok()) {
            rocksdb::ColumnFamilyHandle* cf;
            status = dbPtr->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), "reputation_cf", &cf);
            if(status.ok()) { dbPtr->DestroyColumnFamilyHandle(cf); }
            dbPtr->Close(); delete dbPtr;
            status = rocksdb::DB::Open(options, dbPath_, column_families, &handles, &dbPtr);
        }
        if (!status.ok()) {
            if (err) *err = "Failed to open RocksDB with CFs: " + status.ToString();
            return false;
        }
    }
    defaultCf_ = handles[0]; reputationCf_ = handles[1]; db_.reset(dbPtr);
    return true;
}

void ReorgDetector::close() {
    if (db_) {
        if (defaultCf_) { db_->DestroyColumnFamilyHandle(defaultCf_); defaultCf_ = nullptr; }
        if (reputationCf_) { db_->DestroyColumnFamilyHandle(reputationCf_); reputationCf_ = nullptr; }
        db_.reset();
    }
}

bool ReorgDetector::trackBlock(std::uint64_t height, const std::string& blockHash, 
                              std::uint64_t /*timestamp*/) {
    if (!db_) {
        return false;
    }
    
    std::string key = makeBlockKey(height);
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, blockHash);
    return status.ok();
}

std::optional<ReorgEvent> ReorgDetector::detectReorg(std::uint64_t height, 
                                                     const std::string& newBlockHash,
                                                     std::uint64_t timestamp) {
    if (!db_) {
        return std::nullopt;
    }
    
    std::string key = makeBlockKey(height);
    std::string oldBlockHash;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &oldBlockHash);
    
    if (status.IsNotFound()) {
        // No previous block at this height - not a reorg, just new block
        trackBlock(height, newBlockHash, timestamp);
        return std::nullopt;
    }
    
    if (!status.ok()) {
        // Some other error occurred
        return std::nullopt;
    }
    
    if (oldBlockHash != newBlockHash) {
        // Reorg detected!
        ReorgEvent event;
        event.reorgHeight = height;
        event.oldBlockHash = oldBlockHash;
        event.newBlockHash = newBlockHash;
        event.detectedAtTime = timestamp;
        
        // Handle the reorg and get invalidated anchors
        event.invalidatedAnchors = handleReorg(height);
        
        // Store the reorg event
        storeReorgEvent(event, nullptr);
        
        // Update to new block hash
        trackBlock(height, newBlockHash, timestamp);
        
        // Trigger callback if set
        if (reorgCallback_) {
            reorgCallback_(event);
        }
        
        return event;
    }
    
    return std::nullopt;
}

std::string ReorgDetector::serializeAnchor(const AnchorCommitmentRecord& anchor) const {
    std::ostringstream oss;
    oss << anchor.anchorHash << "|"
        << anchor.bitcoinTxId << "|"
        << anchor.bitcoinHeight << "|"
        << anchor.confirmations << "|"
        << anchor.broadcastTime << "|"
        << anchor.retryCount << "|"
        << static_cast<int>(anchor.status) << "|"
        << anchor.l2StateRoot;
    return oss.str();
}

std::optional<AnchorCommitmentRecord> ReorgDetector::deserializeAnchor(const std::string& data) const {
    std::istringstream iss(data);
    AnchorCommitmentRecord anchor;
    std::string statusStr;
    
    char delim;
    if (!(std::getline(iss, anchor.anchorHash, '|') &&
          std::getline(iss, anchor.bitcoinTxId, '|') &&
          iss >> anchor.bitcoinHeight >> delim &&
          iss >> anchor.confirmations >> delim &&
          iss >> anchor.broadcastTime >> delim &&
          iss >> anchor.retryCount >> delim &&
          std::getline(iss, statusStr, '|') &&
          std::getline(iss, anchor.l2StateRoot))) {
        return std::nullopt;
    }
    
    try {
        anchor.status = static_cast<AnchorStatus>(std::stoi(statusStr));
    } catch (...) {
        return std::nullopt;
    }
    
    return anchor;
}

std::string ReorgDetector::serializeReorgEvent(const ReorgEvent& event) const {
    std::ostringstream oss;
    oss << event.reorgHeight << "|"
        << event.oldBlockHash << "|"
        << event.newBlockHash << "|"
        << event.detectedAtTime << "|"
        << event.invalidatedAnchors.size();
    
    for (const auto& anchor : event.invalidatedAnchors) {
        oss << "|" << anchor;
    }
    
    return oss.str();
}

std::optional<ReorgEvent> ReorgDetector::deserializeReorgEvent(const std::string& data) const {
    std::istringstream iss(data);
    ReorgEvent event;
    std::size_t anchorCount;
    char delim;
    
    if (!(iss >> event.reorgHeight >> delim &&
          std::getline(iss, event.oldBlockHash, '|') &&
          std::getline(iss, event.newBlockHash, '|') &&
          iss >> event.detectedAtTime >> delim &&
          iss >> anchorCount)) {
        return std::nullopt;
    }
    
    event.invalidatedAnchors.reserve(anchorCount);
    for (std::size_t i = 0; i < anchorCount; ++i) {
        std::string anchorHash;
        if (!(iss >> delim && std::getline(iss, anchorHash, '|'))) {
            if (i == anchorCount - 1 && iss.eof()) {
                // Last item might not have trailing delimiter
                iss.clear();
                iss >> anchorHash;
            } else {
                return std::nullopt;
            }
        }
        event.invalidatedAnchors.push_back(anchorHash);
    }
    
    return event;
}

bool ReorgDetector::registerAnchor(const AnchorCommitmentRecord& anchor, std::string* err) {
    if (!db_) {
        if (err) *err = "Database not initialized";
        return false;
    }
    
    std::string key = makeAnchorKey(anchor.anchorHash);
    std::string value = serializeAnchor(anchor);
    
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
    if (!status.ok()) {
        if (err) *err = "Failed to store anchor: " + status.ToString();
        return false;
    }
    
    return true;
}

bool ReorgDetector::updateAnchorConfirmations(const std::string& anchorHash, 
                                             std::uint64_t confirmations,
                                             std::string* err) {
    auto anchorOpt = getAnchorStatus(anchorHash);
    if (!anchorOpt) {
        if (err) *err = "Anchor not found: " + anchorHash;
        return false;
    }
    
    auto anchor = anchorOpt.value();
    anchor.confirmations = confirmations;
    
    if (confirmations >= confirmationThreshold_ && anchor.status == AnchorStatus::PENDING) {
        anchor.status = AnchorStatus::CONFIRMED;
    }
    
    return registerAnchor(anchor, err);
}

std::vector<std::string> ReorgDetector::handleReorg(std::uint64_t reorgHeight) {
    std::vector<std::string> invalidatedAnchors;
    
    if (!db_) {
        return invalidatedAnchors;
    }
    
    // Iterate through all anchors to find those at or after reorg height
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    std::string anchorPrefix = kAnchorPrefix;
    
    for (it->Seek(anchorPrefix); it->Valid() && it->key().ToString().substr(0, anchorPrefix.size()) == anchorPrefix; it->Next()) {
        auto anchorOpt = deserializeAnchor(it->value().ToString());
        if (!anchorOpt) {
            continue;
        }
        
        auto anchor = anchorOpt.value();
        if (anchor.bitcoinHeight >= reorgHeight && anchor.status != AnchorStatus::INVALIDATED_REORG) {
            // Invalidate this anchor
            anchor.status = AnchorStatus::INVALIDATED_REORG;
            anchor.confirmations = 0;
            
            registerAnchor(anchor, nullptr);
            invalidatedAnchors.push_back(anchor.anchorHash);
        }
    }
    
    delete it;
    return invalidatedAnchors;
}

std::vector<AnchorCommitmentRecord> ReorgDetector::getOrphanedAnchors(std::uint64_t currentTime) const {
    std::vector<AnchorCommitmentRecord> orphaned;
    
    if (!db_) {
        return orphaned;
    }
    
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    std::string anchorPrefix = kAnchorPrefix;
    
    for (it->Seek(anchorPrefix); it->Valid() && it->key().ToString().substr(0, anchorPrefix.size()) == anchorPrefix; it->Next()) {
        auto anchorOpt = deserializeAnchor(it->value().ToString());
        if (!anchorOpt) {
            continue;
        }
        
        auto anchor = anchorOpt.value();
        std::uint64_t age = currentTime - anchor.broadcastTime;
        
        if (anchor.status == AnchorStatus::PENDING && 
            age > maxAnchorPendingTime_ && 
            anchor.confirmations == 0) {
            orphaned.push_back(anchor);
        }
    }
    
    delete it;
    return orphaned;
}

std::optional<AnchorCommitmentRecord> ReorgDetector::getAnchorStatus(const std::string& anchorHash) const {
    if (!db_) {
        return std::nullopt;
    }
    
    std::string key = makeAnchorKey(anchorHash);
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
    
    if (status.IsNotFound()) {
        return std::nullopt;
    }
    
    if (!status.ok()) {
        // Some other error occurred
        return std::nullopt;
    }
    
    return deserializeAnchor(value);
}

bool ReorgDetector::updateAnchorStatus(const std::string& anchorHash, AnchorStatus newStatus,
                                      std::string* err) {
    auto anchorOpt = getAnchorStatus(anchorHash);
    if (!anchorOpt) {
        if (err) *err = "Anchor not found: " + anchorHash;
        return false;
    }
    
    auto anchor = anchorOpt.value();
    anchor.status = newStatus;
    
    return registerAnchor(anchor, err);
}

bool ReorgDetector::storeReorgEvent(const ReorgEvent& event, std::string* err) {
    if (!db_) {
        if (err) *err = "Database not initialized";
        return false;
    }
    
    // Get current counter
    std::string counterStr;
    std::uint64_t counter = 0;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), kReorgCounterKey, &counterStr);
    if (status.ok()) {
        try {
            counter = std::stoull(counterStr);
        } catch (...) {
            counter = 0;
        }
    }
    
    // Store the event
    std::string eventKey = makeReorgEventKey(counter);
    std::string eventValue = serializeReorgEvent(event);
    
    // Use write batch to update both event and counter atomically
    rocksdb::WriteBatch batch;
    batch.Put(eventKey, eventValue);
    batch.Put(kReorgCounterKey, std::to_string(counter + 1));
    
    status = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!status.ok()) {
        if (err) *err = "Failed to store reorg event: " + status.ToString();
        return false;
    }
    
    return true;
}

std::vector<ReorgEvent> ReorgDetector::getReorgHistory() const {
    return getRecentReorgHistory(std::numeric_limits<std::size_t>::max());
}

std::vector<ReorgEvent> ReorgDetector::getRecentReorgHistory(std::size_t maxEvents) const {
    std::vector<ReorgEvent> history;
    
    if (!db_) {
        return history;
    }
    
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    std::string reorgPrefix = kReorgEventPrefix;
    
    // Collect all events first
    std::vector<std::pair<std::uint64_t, ReorgEvent>> allEvents;
    for (it->Seek(reorgPrefix); it->Valid() && it->key().ToString().substr(0, reorgPrefix.size()) == reorgPrefix; it->Next()) {
        auto eventOpt = deserializeReorgEvent(it->value().ToString());
        if (eventOpt) {
            // Extract event ID from key
            std::string key = it->key().ToString();
            std::uint64_t eventId = 0;
            try {
                eventId = std::stoull(key.substr(reorgPrefix.size()));
            } catch (...) {
                continue;
            }
            allEvents.emplace_back(eventId, eventOpt.value());
        }
    }
    delete it;
    
    // Sort by event ID descending (most recent first)
    std::sort(allEvents.begin(), allEvents.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Take up to maxEvents
    history.reserve(std::min(maxEvents, allEvents.size()));
    for (std::size_t i = 0; i < std::min(maxEvents, allEvents.size()); ++i) {
        history.push_back(allEvents[i].second);
    }
    
    return history;
}

bool ReorgDetector::pruneOldBlocks(std::uint64_t keepLastN, std::string* err) {
    if (!db_) {
        if (err) *err = "Database not initialized";
        return false;
    }
    
    // Find all block heights
    std::vector<std::uint64_t> heights;
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    std::string blockPrefix = kBlockHashPrefix;
    
    for (it->Seek(blockPrefix); it->Valid() && it->key().ToString().substr(0, blockPrefix.size()) == blockPrefix; it->Next()) {
        auto heightOpt = parseHeightFromKey(it->key().ToString());
        if (heightOpt) {
            heights.push_back(heightOpt.value());
        }
    }
    delete it;
    
    if (heights.size() <= keepLastN) {
        return true; // Nothing to prune
    }
    
    // Sort heights and determine cutoff
    std::sort(heights.begin(), heights.end());
    std::uint64_t cutoffHeight = heights[heights.size() - keepLastN];
    
    // Delete blocks below cutoff
    rocksdb::WriteBatch batch;
    for (std::uint64_t height : heights) {
        if (height < cutoffHeight) {
            batch.Delete(makeBlockKey(height));
        }
    }
    
    rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!status.ok()) {
        if (err) *err = "Failed to prune old blocks: " + status.ToString();
        return false;
    }
    
    return true;
}

std::optional<std::string> ReorgDetector::getBlockHashAtHeight(std::uint64_t height) const {
    if (!db_) {
        return std::nullopt;
    }
    
    std::string key = makeBlockKey(height);
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
    
    if (status.IsNotFound()) {
        return std::nullopt;
    }
    
    if (!status.ok()) {
        // Some other error occurred
        return std::nullopt;
    }
    
    return value;
}

std::vector<AnchorCommitmentRecord> ReorgDetector::getAnchorsByStatus(AnchorStatus status) const {
    std::vector<AnchorCommitmentRecord> result;
    
    if (!db_) {
        return result;
    }
    
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    std::string anchorPrefix = kAnchorPrefix;
    
    for (it->Seek(anchorPrefix); it->Valid() && it->key().ToString().substr(0, anchorPrefix.size()) == anchorPrefix; it->Next()) {
        auto anchorOpt = deserializeAnchor(it->value().ToString());
        if (anchorOpt && anchorOpt.value().status == status) {
            result.push_back(anchorOpt.value());
        }
    }
    
    delete it;
    return result;
}


bool ReorgDetector::setReputation(const std::string& peerId, const std::string& repData, std::string* err) {
    if (!db_ || !reputationCf_) { if (err) *err = "RocksDB or reputation_cf not initialized"; return false; }
    rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), reputationCf_, peerId, repData);
    if (!s.ok() && err) *err = s.ToString();
    return s.ok();
}
std::optional<std::string> ReorgDetector::getReputation(const std::string& peerId) const {
    if (!db_ || !reputationCf_) return std::nullopt;
    std::string val;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), reputationCf_, peerId, &val);
    if (s.ok()) return val;
    return std::nullopt;
}

} // namespace ailee::l1
