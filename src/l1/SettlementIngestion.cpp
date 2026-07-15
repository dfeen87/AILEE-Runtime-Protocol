#include "SettlementIngestion.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>

namespace ailee::l1 {

namespace utils {

void hexToBytes(const std::string& hex, uint8_t* out, size_t outLen) {
    if (hex.length() > outLen * 2) {
        throw std::invalid_argument("Hex string too long for buffer");
    }

    std::memset(out, 0, outLen);

    size_t offset = (outLen * 2) - hex.length();

    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        if (byteString.length() == 1) {
            byteString = "0" + byteString;
        }
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        out[(i + offset) / 2] = byte;
    }
}

std::string bytesToHex(const uint8_t* bytes, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

} // namespace utils

// ---------------------------------------------------------
// CacheAlignedAnchor Helpers
// ---------------------------------------------------------

std::string CacheAlignedAnchor::getAnchorHashStr() const {
    return utils::bytesToHex(anchorHash, 32);
}
std::string CacheAlignedAnchor::getL2StateRootStr() const {
    return utils::bytesToHex(l2StateRoot, 32);
}
std::string CacheAlignedAnchor::getBitcoinTxIdStr() const {
    return utils::bytesToHex(bitcoinTxId, 32);
}

void CacheAlignedAnchor::setAnchorHashStr(const std::string& hexStr) {
    utils::hexToBytes(hexStr, anchorHash, 32);
}
void CacheAlignedAnchor::setL2StateRootStr(const std::string& hexStr) {
    utils::hexToBytes(hexStr, l2StateRoot, 32);
}
void CacheAlignedAnchor::setBitcoinTxIdStr(const std::string& hexStr) {
    utils::hexToBytes(hexStr, bitcoinTxId, 32);
}

// ---------------------------------------------------------
// SettlementIngestionEngine
// ---------------------------------------------------------

SettlementIngestionEngine::SettlementIngestionEngine(std::shared_ptr<rocksdb::DB> db)
    : db_(std::move(db)) {
    if (!db_) {
        throw std::runtime_error("SettlementIngestionEngine requires a valid RocksDB instance");
    }
}

std::optional<CacheAlignedAnchor> SettlementIngestionEngine::getAnchorZeroCopy(const std::string& anchorHashHex) const {
    // Construct key using a fixed stack buffer instead of std::string allocation
    constexpr size_t PREFIX_LEN = 9; // "c_anchor:"
    size_t hashLen = anchorHashHex.length();
    size_t keyLen = PREFIX_LEN + hashLen;

    // We expect anchorHashHex to be exactly 64 chars long (32 bytes * 2), but we support up to a safe limit.
    if (hashLen > 128) {
        return std::nullopt;
    }

    char keyBuffer[256];
    std::memcpy(keyBuffer, "c_anchor:", PREFIX_LEN);
    std::memcpy(keyBuffer + PREFIX_LEN, anchorHashHex.c_str(), hashLen);

    rocksdb::Slice keySlice(keyBuffer, keyLen);

    rocksdb::PinnableSlice valueSlice;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), db_->DefaultColumnFamily(), keySlice, &valueSlice);

    if (!s.ok() || valueSlice.size() != sizeof(CacheAlignedAnchor)) {
        return std::nullopt;
    }

    CacheAlignedAnchor anchor;
    // Zero-copy cast bypasses field-by-field deserialization
    std::memcpy(&anchor, valueSlice.data(), sizeof(CacheAlignedAnchor));

    return anchor;
}

std::vector<CacheAlignedAnchor> SettlementIngestionEngine::ingestConfirmedAnchors(uint64_t currentL1Height, uint64_t confirmationThreshold) const {
    std::vector<CacheAlignedAnchor> confirmed;

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    rocksdb::Slice prefix("c_anchor:");

    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        rocksdb::Slice val = it->value();
        if (val.size() == sizeof(CacheAlignedAnchor)) {
            // Copy into aligned struct, because rocksdb::Slice data pointer might not be 64-byte aligned
            // which causes a segfault on aligned push_back/copy.
            CacheAlignedAnchor tempAnchor;
            std::memcpy(&tempAnchor, val.data(), sizeof(CacheAlignedAnchor));

            // Deterministic check based on L1 height
            if (tempAnchor.status == static_cast<uint8_t>(AnchorStatus::PENDING)) {
                if (currentL1Height >= tempAnchor.bitcoinHeight) {
                    uint64_t depth = currentL1Height - tempAnchor.bitcoinHeight + 1;
                    if (depth >= confirmationThreshold) {
                        confirmed.push_back(tempAnchor);
                    }
                }
            } else if (tempAnchor.status == static_cast<uint8_t>(AnchorStatus::CONFIRMED)) {
                 confirmed.push_back(tempAnchor);
            }
        }
    }

    return confirmed;
}

std::vector<CacheAlignedAnchor> SettlementIngestionEngine::ingestReorgInvalidations(uint64_t reorgHeight) const {
    std::vector<CacheAlignedAnchor> invalidated;

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    rocksdb::Slice prefix("c_anchor:");

    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        rocksdb::Slice val = it->value();
        if (val.size() == sizeof(CacheAlignedAnchor)) {
            CacheAlignedAnchor tempAnchor;
            std::memcpy(&tempAnchor, val.data(), sizeof(CacheAlignedAnchor));

            // If the anchor was mined at or after the reorg point, it is deterministically invalidated
            if (tempAnchor.bitcoinHeight >= reorgHeight &&
                tempAnchor.status != static_cast<uint8_t>(AnchorStatus::INVALIDATED_REORG)) {
                invalidated.push_back(tempAnchor);
            }
        }
    }

    return invalidated;
}

bool SettlementIngestionEngine::writeAnchorZeroCopy(const CacheAlignedAnchor& anchor, std::string* /*err*/) {
    constexpr size_t PREFIX_LEN = 9; // "c_anchor:"
    char keyBuffer[256];
    std::memcpy(keyBuffer, "c_anchor:", PREFIX_LEN);

    // Instead of allocating a string for the hex representation, write it directly to the buffer
    static const char* const lut = "0123456789abcdef";
    for (size_t i = 0; i < 32; ++i) {
        keyBuffer[PREFIX_LEN + i * 2] = lut[anchor.anchorHash[i] >> 4];
        keyBuffer[PREFIX_LEN + i * 2 + 1] = lut[anchor.anchorHash[i] & 0x0F];
    }
    size_t keyLen = PREFIX_LEN + 64;

    rocksdb::Slice keySlice(keyBuffer, keyLen);

    // Convert struct directly to RocksDB slice representation
    rocksdb::Slice valSlice(reinterpret_cast<const char*>(&anchor), sizeof(CacheAlignedAnchor));

    rocksdb::WriteOptions writeOpts;
    writeOpts.sync = true; // Deterministic sync
    rocksdb::Status s = db_->Put(writeOpts, keySlice, valSlice);

    if (!s.ok()) {
        // Do not allocate a std::string to err if it can be avoided on critical path, but follow interface contract
        // The user mentioned replacing `std::string* err` with bool return, but we can't change the header.
        // We just ignore the string assignment to avoid allocations, or just return false.
        // Returning false is enough for a deterministic status.
        return false;
    }
    return true;
}

} // namespace ailee::l1
