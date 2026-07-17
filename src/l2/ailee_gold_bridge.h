/**
 * AILEE Bitcoin-to-Gold Conversion Bridge
 * 
 * A secure, autonomous system for converting Bitcoin to physical gold
 * with proof-of-burn mechanics, oracle pricing, and tokenized gold receipts.
 * 
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_GOLD_BRIDGE_H
#define AILEE_GOLD_BRIDGE_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>
#include <optional>
#include <chrono>
#include <functional>
#include <algorithm>
#include <cmath>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <nlohmann/json.hpp>
#include "../storage/PersistentStorage.h"
#include "../rpc/BitcoinRPCClient.h"


namespace ailee {

// Configuration constants
constexpr double BTC_TO_SATOSHI = 100000000.0;
constexpr size_t MIN_CONFIRMATIONS = 6;
constexpr uint64_t ORACLE_TIMEOUT_SECONDS = 300; // 5 minutes
constexpr double CONVERSION_FEE_PERCENT = 0.5; // 0.5% fee

// Gold denominations in troy ounces
enum class GoldDenomination {
    ONE_TENTH_OZ = 0,    // 0.1 oz
    QUARTER_OZ = 1,      // 0.25 oz
    HALF_OZ = 2,         // 0.5 oz
    ONE_OZ = 3,          // 1.0 oz
    FIVE_OZ = 4,         // 5.0 oz
    TEN_OZ = 5           // 10.0 oz
};

// Forward declarations
class PriceOracle;
class GoldInventory;
class ConversionTransaction;
class ProofOfBurn;
class TokenizedGold;

/**
 * Price Oracle System
 * Multi-source price aggregation with failover
 */
class PriceOracle {
public:
    struct PriceData {
        double btcUsdPrice;
        double goldUsdPrice;
        uint64_t timestamp;
        std::vector<std::string> sources;
        double confidence;
    };

    using OracleCallback = std::function<PriceData()>;

    void registerOracle(const std::string& name, OracleCallback callback) {
        oracles_[name] = callback;
    }

    PriceData getAggregatedPrice() {
        std::vector<PriceData> prices;
        
        for (const auto& oracle : oracles_) {
            try {
                PriceData data = oracle.second();
                
                // Validate freshness
                uint64_t currentTime = getCurrentTimestamp();
                if (currentTime - data.timestamp < ORACLE_TIMEOUT_SECONDS) {
                    prices.push_back(data);
                }
            } catch (...) {
                // Oracle failed, continue with others
                continue;
            }
        }

        if (prices.empty()) {
            throw std::runtime_error("No valid oracle data available");
        }

        // Calculate median prices for robustness
        return calculateMedianPrice(prices);
    }

    double getBtcToGoldRate() {
        PriceData price = getAggregatedPrice();
        return price.btcUsdPrice / price.goldUsdPrice;
    }

private:
    std::map<std::string, OracleCallback> oracles_;

    uint64_t getCurrentTimestamp() const {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    PriceData calculateMedianPrice(std::vector<PriceData>& prices) {
        if (prices.empty()) {
            throw std::runtime_error("No prices to aggregate");
        }

        // Sort by BTC price
        std::sort(prices.begin(), prices.end(),
            [](const PriceData& a, const PriceData& b) {
                return a.btcUsdPrice < b.btcUsdPrice;
            });

        size_t mid = prices.size() / 2;
        PriceData median = prices[mid];

        // Calculate confidence based on price variance
        double variance = 0.0;
        for (const auto& p : prices) {
            variance += std::abs(p.btcUsdPrice - median.btcUsdPrice);
        }
        median.confidence = 1.0 - (variance / (median.btcUsdPrice * static_cast<double>(prices.size())));

        // Aggregate sources
        for (const auto& p : prices) {
            median.sources.insert(median.sources.end(),
                p.sources.begin(), p.sources.end());
        }

        median.timestamp = getCurrentTimestamp();
        return median;
    }
};

/**
 * Gold Inventory Management
 * Tracks physical gold stock across multiple secure locations
 */
class GoldInventory {
public:
    struct InventoryItem {
        std::string serialNumber;
        GoldDenomination denomination;
        double weightOz;
        std::string location;
        bool available;
        uint64_t lastAuditTimestamp;
    };

    struct LocationInventory {
        std::string locationId;
        std::string address;
        std::vector<InventoryItem> items;
        double totalWeightOz;
        uint64_t lastRestockTimestamp;
    };

    void addLocation(const std::string& locationId, const std::string& address) {
        LocationInventory loc;
        loc.locationId = locationId;
        loc.address = address;
        loc.totalWeightOz = 0.0;
        loc.lastRestockTimestamp = getCurrentTimestamp();

        storage_->put("gold_inv_loc_" + locationId, loc_to_json(loc).dump());
        auto idxOpt = storage_->get("gold_inv_locs_index");
        std::vector<std::string> locs;
        if (idxOpt) {
            auto arr_j = nlohmann::json::parse(*idxOpt);
            for (const auto& val : arr_j) locs.push_back(val.get<std::string>());
        }
        if (std::find(locs.begin(), locs.end(), locationId) == locs.end()) {
            locs.push_back(locationId);
            nlohmann::json locs_arr = nlohmann::json::array_t{};
            for(const auto& str : locs) locs_arr.push_back(str);
            storage_->put("gold_inv_locs_index", locs_arr.dump());
        }
    }

    bool addGoldCoin(const std::string& locationId, 
                     const InventoryItem& item) {
        auto locOpt = storage_->get("gold_inv_loc_" + locationId);
        if (!locOpt) return false;
        auto loc = loc_from_json(nlohmann::json::parse(*locOpt));
        loc.items.push_back(item);
        loc.totalWeightOz += item.weightOz;
        storage_->put("gold_inv_loc_" + locationId, loc_to_json(loc).dump());
        return true;
    }

    std::optional<InventoryItem> reserveGold(GoldDenomination denom,
                                             const std::string& preferredLocation = "") {
        if (!preferredLocation.empty()) {
            auto item = findAndReserve(preferredLocation, denom);
            if (item.has_value()) return item;
        }
        for (const auto& locId : getLocations()) {
            auto item = findAndReserve(locId, denom);
            if (item.has_value()) return item;
        }
        return std::nullopt;
    }

    double getAvailableWeight(const std::string& locationId) const {
        auto locOpt = storage_->get("gold_inv_loc_" + locationId);
        if (!locOpt) return 0.0;
        auto loc = loc_from_json(nlohmann::json::parse(*locOpt));
        double available = 0.0;
        for (const auto& item : loc.items) {
            if (item.available) {
                available += item.weightOz;
            }
        }
        return available;
    }

    std::vector<std::string> getLocations() const {
        auto idxOpt = storage_->get("gold_inv_locs_index");
        std::vector<std::string> locs;
        if (!idxOpt) return locs;
        auto arr_j = nlohmann::json::parse(*idxOpt);
        for (const auto& val : arr_j) locs.push_back(val.get<std::string>());
        return locs;
    }

    bool markAsDispensed(const std::string& serialNumber) {
        auto op = getMarkAsDispensedOp(serialNumber);
        if (op) return storage_->put(op->key, op->value);
        return false;
    }

public:
    GoldInventory(ailee::storage::PersistentStorage* storage) : storage_(storage) {
        if (!storage_) throw std::runtime_error("Storage uninitialized");
    }

    nlohmann::json item_to_json(const InventoryItem& i) const {
        nlohmann::json j = nlohmann::json();
        j["serialNumber"] = i.serialNumber;
        j["denomination"] = static_cast<int>(i.denomination);
        j["weightOz"] = i.weightOz;
        j["location"] = i.location;
        j["available"] = i.available;
        j["lastAuditTimestamp"] = static_cast<double>(i.lastAuditTimestamp);
        return j;
    }

    InventoryItem item_from_json(const nlohmann::json& j) const {
        InventoryItem i;
        i.serialNumber = j.value("serialNumber", std::string());
        i.denomination = static_cast<GoldDenomination>(static_cast<int>(j.value("denomination", 0.0)));
        i.weightOz = j.value("weightOz", 0.0);
        i.location = j.value("location", std::string());
        i.available = j.value("available", false);
        i.lastAuditTimestamp = static_cast<uint64_t>(j.value("lastAuditTimestamp", 0.0));
        return i;
    }

    nlohmann::json loc_to_json(const LocationInventory& l) const {
        nlohmann::json j = nlohmann::json();
        j["locationId"] = l.locationId;
        j["address"] = l.address;

        nlohmann::json items_arr = nlohmann::json::array_t{};
        for (const auto& item : l.items) {
            items_arr.push_back(item_to_json(item));
        }
        j["items"] = items_arr;
        j["totalWeightOz"] = l.totalWeightOz;
        j["lastRestockTimestamp"] = static_cast<double>(l.lastRestockTimestamp);
        return j;
    }

    LocationInventory loc_from_json(const nlohmann::json& j) const {
        LocationInventory l;
        if (j.contains("locationId")) l.locationId = j["locationId"].get<std::string>();
        if (j.contains("address")) l.address = j["address"].get<std::string>();

        if (j.contains("items")) {
            for (const auto& item_j : j["items"]) {
                l.items.push_back(item_from_json(item_j));
            }
        }

        if (j.contains("totalWeightOz")) l.totalWeightOz = j["totalWeightOz"].get<double>();
        if (j.contains("lastRestockTimestamp")) l.lastRestockTimestamp = static_cast<uint64_t>(j["lastRestockTimestamp"].get<double>());
        return l;
    }

    std::optional<ailee::storage::PersistentStorage::BatchOp> getMarkAsDispensedOp(const std::string& serialNumber) {
        if (!storage_) throw std::runtime_error("Storage uninitialized");
        for (const auto& locId : getLocations()) {
            auto locOpt = storage_->get("gold_inv_loc_" + locId);
            if (!locOpt) continue;

            auto loc = loc_from_json(nlohmann::json::parse(*locOpt));
            bool found = false;
            for (auto& item : loc.items) {
                if (item.serialNumber == serialNumber) {
                    item.available = false;
                    found = true;
                    break;
                }
            }
            if (found) {
                ailee::storage::PersistentStorage::BatchOp op;
                op.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
                op.key = "gold_inv_loc_" + locId;
                op.value = loc_to_json(loc).dump();
                return op;
            }
        }
        return std::nullopt;
    }

private:
    ailee::storage::PersistentStorage* storage_;

    uint64_t getCurrentTimestamp() const {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    std::optional<InventoryItem> findAndReserve(const std::string& locationId,
                                                GoldDenomination denom) {
        auto locOpt = storage_->get("gold_inv_loc_" + locationId);
        if (!locOpt) return std::nullopt;
        auto loc = loc_from_json(nlohmann::json::parse(*locOpt));
        for (auto& item : loc.items) {
            if (item.available && item.denomination == denom) return item;
        }
        return std::nullopt;
    }
};

/**
 * Proof of Burn Implementation
 * Cryptographically provable Bitcoin destruction
 */
class ProofOfBurn {
public:
    struct BurnProof {
        std::string txId;
        uint32_t voutIndex;
        uint64_t amountSatoshis;
        std::string burnAddress;
        uint64_t blockHeight;
        uint64_t timestamp;
        std::vector<uint8_t> merkleProof;
        bool verified;
    };

    static BurnProof createBurnProof(
        const std::string& txId,
        uint32_t vout,
        uint64_t amount,
        uint64_t blockHeight
    ) {
        BurnProof proof;
        proof.txId = txId;
        proof.voutIndex = vout;
        proof.amountSatoshis = amount;
        proof.burnAddress = generateBurnAddress();
        proof.blockHeight = blockHeight;
        proof.timestamp = getCurrentTimestamp();
        proof.verified = false;

        // Generate Merkle proof (simplified)
        std::string proofData = txId + std::to_string(vout) + std::to_string(amount);
        proof.merkleProof = sha256Hash(
            std::vector<uint8_t>(proofData.begin(), proofData.end())
        );

        return proof;
    }

    static bool verifyBurnProof(const BurnProof& proof, size_t minConfirmations, ailee::BitcoinRPCClient* rpcClient = nullptr) {
        if (!isValidBurnAddress(proof.burnAddress)) return false;

        std::string proofData = proof.txId + std::to_string(proof.voutIndex) + std::to_string(proof.amountSatoshis);
        auto expectedHash = sha256Hash(std::vector<uint8_t>(proofData.begin(), proofData.end()));

        if (expectedHash != proof.merkleProof) return false;

        if (rpcClient) {
            long confirmations = rpcClient->getTxConfirmations(proof.txId);
            if (confirmations < 0) return false;
            if (static_cast<size_t>(confirmations) < minConfirmations) return false;
            return true;
        }

        return false;
    }

private:
    static std::string generateBurnAddress() {
        // OP_RETURN or provably unspendable address
        return "1BitcoinEaterAddressDontSendf59kuE";
    }

    static inline bool isValidBurnAddress(const std::string& address) {
        // Verify it's a known burn address pattern
        return address.find("BitcoinEater") != std::string::npos ||
               address.find("1111111111111111111114oLvT2") != std::string::npos;
    }

    static inline uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    static inline std::vector<uint8_t> sha256Hash(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
        SHA256(data.data(), data.size(), hash.data());
        return hash;
    }
};

/**
 * Tokenized Gold (wGOLD)
 * Redeemable digital certificates for physical gold
 */
class TokenizedGold {
public:
    struct GoldToken {
        std::string tokenId;
        std::string ownerAddress;
        double weightOz;
        GoldDenomination denomination;
        std::string backedBySerial;
        std::string custodianLocation;
        uint64_t issuanceTimestamp;
        bool redeemed;
    };

    std::string issueToken(
        const std::string& ownerAddress,
        const GoldInventory::InventoryItem& backingGold
    ) {
        auto opsPair = getIssueTokenOps(ownerAddress, backingGold);
        storage_->executeBatch(opsPair.second);
        return opsPair.first;
    }

    bool redeemToken(const std::string& tokenId, const std::string& claimant) {
        auto tokenOpt = storage_->get("gold_token_" + tokenId);
        if (!tokenOpt) return false;
        
        auto token = token_from_json(nlohmann::json::parse(*tokenOpt));
        if (token.redeemed) return false;
        if (token.ownerAddress != claimant) return false;

        std::string oldOwner = token.ownerAddress;
        token.redeemed = true;
        token.ownerAddress = "REDEEMED";
        
        std::vector<ailee::storage::PersistentStorage::BatchOp> ops;
        ailee::storage::PersistentStorage::BatchOp tokenOp;
        tokenOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
        tokenOp.key = "gold_token_" + tokenId;
        tokenOp.value = token_to_json(token).dump();
        ops.push_back(tokenOp);

        auto ownerTokensOpt = storage_->get("gold_owner_" + oldOwner);
        if (ownerTokensOpt) {
            auto arr_j = nlohmann::json::parse(*ownerTokensOpt);
            std::vector<std::string> owned;
            for (const auto& val : arr_j) owned.push_back(val.get<std::string>());

            auto it = std::find(owned.begin(), owned.end(), tokenId);
            if (it != owned.end()) {
                owned.erase(it);
                nlohmann::json owned_arr = nlohmann::json::array_t{};
                for(const auto& str : owned) owned_arr.push_back(str);
                ailee::storage::PersistentStorage::BatchOp ownerOp;
                ownerOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
                ownerOp.key = "gold_owner_" + oldOwner;
                ownerOp.value = owned_arr.dump();
                ops.push_back(ownerOp);
            }
        }
        return storage_->executeBatch(ops);
    }

    std::optional<GoldToken> getToken(const std::string& tokenId) const {
        auto tokenOpt = storage_->get("gold_token_" + tokenId);
        if (!tokenOpt) return std::nullopt;
        return token_from_json(nlohmann::json::parse(*tokenOpt));
    }

    std::vector<GoldToken> getTokensByOwner(const std::string& owner) const {
        std::vector<GoldToken> result;
        auto ownerTokensOpt = storage_->get("gold_owner_" + owner);
        if (!ownerTokensOpt) return result;

        auto arr_j = nlohmann::json::parse(*ownerTokensOpt);
        for (const auto& val : arr_j) {
            auto tokenId = val.get<std::string>();
            auto token = getToken(tokenId);
            if (token && !token->redeemed) {
                result.push_back(*token);
            }
        }
        return result;
    }

public:
    TokenizedGold(ailee::storage::PersistentStorage* storage) : storage_(storage) {
        if (!storage_) throw std::runtime_error("Storage uninitialized");
    }

    nlohmann::json token_to_json(const GoldToken& t) const {
        nlohmann::json j = nlohmann::json();
        j["tokenId"] = t.tokenId;
        j["ownerAddress"] = t.ownerAddress;
        j["weightOz"] = t.weightOz;
        j["denomination"] = static_cast<int>(t.denomination);
        j["backedBySerial"] = t.backedBySerial;
        j["custodianLocation"] = t.custodianLocation;
        j["issuanceTimestamp"] = static_cast<double>(t.issuanceTimestamp);
        j["redeemed"] = t.redeemed;
        return j;
    }

    GoldToken token_from_json(const nlohmann::json& j) const {
        GoldToken t;
        if (j.contains("tokenId")) t.tokenId = j["tokenId"].get<std::string>();
        if (j.contains("ownerAddress")) t.ownerAddress = j["ownerAddress"].get<std::string>();
        if (j.contains("weightOz")) t.weightOz = j["weightOz"].get<double>();
        if (j.contains("denomination")) t.denomination = static_cast<GoldDenomination>(static_cast<int>(j["denomination"].get<double>()));
        if (j.contains("backedBySerial")) t.backedBySerial = j["backedBySerial"].get<std::string>();
        if (j.contains("custodianLocation")) t.custodianLocation = j["custodianLocation"].get<std::string>();
        if (j.contains("issuanceTimestamp")) t.issuanceTimestamp = static_cast<uint64_t>(j["issuanceTimestamp"].get<double>());
        if (j.contains("redeemed")) t.redeemed = j["redeemed"].get<bool>();
        return t;
    }

    std::pair<std::string, std::vector<ailee::storage::PersistentStorage::BatchOp>> getIssueTokenOps(
        const std::string& ownerAddress,
        const GoldInventory::InventoryItem& backingGold
    ) {
        GoldToken token;
        token.tokenId = generateTokenId(ownerAddress, backingGold.serialNumber);
        token.ownerAddress = ownerAddress;
        token.weightOz = backingGold.weightOz;
        token.denomination = backingGold.denomination;
        token.backedBySerial = backingGold.serialNumber;
        token.custodianLocation = backingGold.location;
        token.issuanceTimestamp = getCurrentTimestamp();
        token.redeemed = false;

        std::vector<ailee::storage::PersistentStorage::BatchOp> ops;
        ailee::storage::PersistentStorage::BatchOp tokenOp;
        tokenOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
        tokenOp.key = "gold_token_" + token.tokenId;
        tokenOp.value = token_to_json(token).dump();
        ops.push_back(tokenOp);

        std::vector<std::string> owned;
        auto ownerTokensOpt = storage_->get("gold_owner_" + ownerAddress);
        if (ownerTokensOpt) {
            auto arr_j = nlohmann::json::parse(*ownerTokensOpt);
            for (const auto& val : arr_j) owned.push_back(val.get<std::string>());
        }
        owned.push_back(token.tokenId);

        nlohmann::json owned_arr = nlohmann::json::array_t{};
        for(const auto& str : owned) owned_arr.push_back(str);

        ailee::storage::PersistentStorage::BatchOp ownerOp;
        ownerOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
        ownerOp.key = "gold_owner_" + ownerAddress;
        ownerOp.value = owned_arr.dump();
        ops.push_back(ownerOp);
        return {token.tokenId, ops};
    }

private:
    ailee::storage::PersistentStorage* storage_;

    uint64_t getCurrentTimestamp() const {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    std::string generateTokenId(const std::string& owner, 
                                const std::string& serial) {
        std::string combined = owner + serial + std::to_string(getCurrentTimestamp());
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
        SHA256(reinterpret_cast<const uint8_t*>(combined.data()),
               combined.size(), hash.data());
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            snprintf(hexStr + (i * 2), 3, "%02x", hash[i]);
        }
        return std::string(hexStr, 64);
    }
};

/**
 * Conversion Transaction
 * Represents a single BTC-to-Gold conversion
 */
class ConversionTransaction {
public:
    enum class Status {
        PENDING_PAYMENT,
        PAYMENT_CONFIRMED,
        PRICE_LOCKED,
        GOLD_RESERVED,
        TOKEN_ISSUED,
        PHYSICAL_DISPENSED,
        COMPLETED,
        FAILED
    };

    struct ConversionData {
        std::string conversionId;
        std::string userAddress;
        uint64_t btcAmountSatoshis;
        double goldWeightOz;
        GoldDenomination denomination;
        double btcPriceUsd;
        double goldPriceUsd;
        uint64_t timestamp;
        Status status;
        ProofOfBurn::BurnProof burnProof;
        std::string goldTokenId;
        std::string goldSerialNumber;
        bool burnOption; // true = burn BTC, false = sell BTC
    };

    ConversionTransaction(const std::string& userAddr, uint64_t btcAmount, bool burn)
        : userAddress_(userAddr), btcAmount_(btcAmount), burnOption_(burn) {
        data_.conversionId = generateConversionId();
        data_.userAddress = userAddr;
        data_.btcAmountSatoshis = btcAmount;
        data_.timestamp = getCurrentTimestamp();
        data_.status = Status::PENDING_PAYMENT;
        data_.burnOption = burn;
    }

    bool processPayment(const std::string& txId, uint32_t vout, uint64_t blockHeight) {
        if (data_.status != Status::PENDING_PAYMENT) return false;

        // Create burn proof if burn option selected
        if (data_.burnOption) {
            data_.burnProof = ProofOfBurn::createBurnProof(
                txId, vout, data_.btcAmountSatoshis, blockHeight
            );
        }

        data_.status = Status::PAYMENT_CONFIRMED;
        return true;
    }

    bool lockPrice(const PriceOracle::PriceData& priceData) {
        if (data_.status != Status::PAYMENT_CONFIRMED) return false;

        data_.btcPriceUsd = priceData.btcUsdPrice;
        data_.goldPriceUsd = priceData.goldUsdPrice;

        // Calculate gold amount with fee
        double btcValue = (static_cast<double>(data_.btcAmountSatoshis) / BTC_TO_SATOSHI) * data_.btcPriceUsd;
        double feeAmount = btcValue * (CONVERSION_FEE_PERCENT / 100.0);
        double netValue = btcValue - feeAmount;
        
        data_.goldWeightOz = netValue / data_.goldPriceUsd;
        data_.status = Status::PRICE_LOCKED;
        
        return true;
    }

    bool reserveGold(GoldInventory& inventory, GoldDenomination denom) {
        if (data_.status != Status::PRICE_LOCKED) return false;

        auto item = inventory.reserveGold(denom);
        if (!item.has_value()) {
            data_.status = Status::FAILED;
            return false;
        }

        data_.goldSerialNumber = item->serialNumber;
        data_.denomination = denom;
        data_.status = Status::GOLD_RESERVED;
        
        return true;
    }

    bool issueToken(TokenizedGold& tokenSystem,
                   const GoldInventory::InventoryItem& backingGold) {
        if (data_.status != Status::GOLD_RESERVED) return false;

        data_.goldTokenId = tokenSystem.issueToken(data_.userAddress, backingGold);
        data_.status = Status::TOKEN_ISSUED;
        
        return true;
    }

    bool markTokenIssued(const std::string& tokenId) {
        if (data_.status != Status::GOLD_RESERVED) return false;
        data_.goldTokenId = tokenId;
        data_.status = Status::TOKEN_ISSUED;
        return true;
    }

    bool completePhysicalDispense() {
        if (data_.status != Status::TOKEN_ISSUED) return false;

        data_.status = Status::PHYSICAL_DISPENSED;
        data_.status = Status::COMPLETED;
        
        return true;
    }

    const ConversionData& getData() const { return data_; }
    Status getStatus() const { return data_.status; }

private:
    std::string userAddress_;
    uint64_t btcAmount_;
    bool burnOption_;
    ConversionData data_;

    uint64_t getCurrentTimestamp() const {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    std::string generateConversionId() {
        std::string combined = userAddress_ + std::to_string(btcAmount_) 
                             + std::to_string(getCurrentTimestamp());
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
        SHA256(reinterpret_cast<const uint8_t*>(combined.data()),
               combined.size(), hash.data());
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            snprintf(hexStr + (i * 2), 3, "%02x", hash[i]);
        }
        return std::string(hexStr, 64);
    }
};

/**
 * Gold Bridge Manager
 * Main interface for BTC-to-Gold conversions
 */
class GoldBridge {
public:
    GoldBridge(ailee::storage::PersistentStorage* storage = nullptr, ailee::BitcoinRPCClient* rpcClient = nullptr)
        : storage_(storage),
          rpcClient_(rpcClient),
          priceOracle_(std::make_unique<PriceOracle>()),
          inventory_(std::make_unique<GoldInventory>(storage)),
          tokenSystem_(std::make_unique<TokenizedGold>(storage)) {}

    std::string initiateConversion(
        const std::string& userAddress,
        uint64_t btcAmountSatoshis,
        bool burnOption = false
    ) {
        auto tx = std::make_shared<ConversionTransaction>(
            userAddress, btcAmountSatoshis, burnOption
        );

        std::string conversionId = tx->getData().conversionId;
        conversions_[conversionId] = tx;
        
        return conversionId;
    }

    bool confirmPayment(
        const std::string& conversionId,
        const std::string& btcTxId,
        uint32_t vout,
        uint64_t blockHeight
    ) {
        auto it = conversions_.find(conversionId);
        if (it == conversions_.end()) return false;

        return it->second->processPayment(btcTxId, vout, blockHeight);
    }

    bool executeConversion(
        const std::string& conversionId,
        GoldDenomination denomination
    ) {
        auto it = conversions_.find(conversionId);
        if (it == conversions_.end()) return false;

        auto& tx = it->second;

        if (tx->getData().burnOption) {
            if (!ProofOfBurn::verifyBurnProof(tx->getData().burnProof, MIN_CONFIRMATIONS, rpcClient_)) {
                return false;
            }
        }

        auto priceData = priceOracle_->getAggregatedPrice();
        if (!tx->lockPrice(priceData)) return false;

        if (!tx->reserveGold(*inventory_, denomination)) return false;

        auto item = inventory_->reserveGold(denomination);
        if (!item.has_value()) return false;
        
        if (storage_) {
            std::vector<ailee::storage::PersistentStorage::BatchOp> batchOps;
            auto dispenseOp = inventory_->getMarkAsDispensedOp(item->serialNumber);
            if (!dispenseOp) return false;
            batchOps.push_back(*dispenseOp);

            auto opsPair = tokenSystem_->getIssueTokenOps(tx->getData().userAddress, item.value());
            batchOps.insert(batchOps.end(), opsPair.second.begin(), opsPair.second.end());

            if (!storage_->executeBatch(batchOps)) return false;
            tx->markTokenIssued(opsPair.first);
        }

        return tx->completePhysicalDispense();
    }

    ConversionTransaction::Status getConversionStatus(
        const std::string& conversionId
    ) const {
        auto it = conversions_.find(conversionId);
        if (it == conversions_.end()) {
            return ConversionTransaction::Status::FAILED;
        }
        return it->second->getStatus();
    }

    PriceOracle* getPriceOracle() { return priceOracle_.get(); }
    GoldInventory* getInventory() { return inventory_.get(); }
    TokenizedGold* getTokenSystem() { return tokenSystem_.get(); }

private:
    ailee::storage::PersistentStorage* storage_;
    ailee::BitcoinRPCClient* rpcClient_;
    std::unique_ptr<PriceOracle> priceOracle_;
    std::unique_ptr<GoldInventory> inventory_;
    std::unique_ptr<TokenizedGold> tokenSystem_;
    std::map<std::string, std::shared_ptr<ConversionTransaction>> conversions_;
};

} // namespace ailee

#endif // AILEE_GOLD_BRIDGE_H
