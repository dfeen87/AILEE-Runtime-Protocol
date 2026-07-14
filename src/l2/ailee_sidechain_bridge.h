/**
 * AILEE Sidechain Bridge
 * 
 * Two-way peg implementation for trustless BTC transfers between Bitcoin
 * mainnet (Layer-1) and AILEE Layer-2. Implements federated peg with
 * multi-signature security, atomic swaps, and SPV proof verification.
 * 
 * Features:
 * - Peg-in: Lock BTC on mainnet → Mint equivalent on AILEE
 * - Peg-out: Burn on AILEE → Release BTC on mainnet
 * - SPV proof verification for trustless validation
 * - Multi-signature federation with Byzantine fault tolerance
 * - Emergency recovery mechanisms
 * - Atomic swap support for trustless exchanges
 * 
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_SIDECHAIN_BRIDGE_H
#define AILEE_SIDECHAIN_BRIDGE_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>
#include <set>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <cctype>
#include "Global_Seven.h"
#include "L2State.h"
#include "zk_proofs.h"
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <secp256k1.h>
#include <nlohmann/json.hpp>
#include "../storage/PersistentStorage.h"

namespace ailee {

inline secp256k1_context* getBridgeSecp256k1VerifyContext() {
    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    return ctx;
}

// Bridge configuration constants
constexpr size_t MIN_CONFIRMATIONS_PEGIN = 6;      // Bitcoin confirmations required
constexpr size_t MIN_CONFIRMATIONS_PEGOUT = 100;   // AILEE confirmations for peg-out
constexpr size_t FEDERATION_SIZE = 15;             // Number of federation signers
constexpr size_t FEDERATION_THRESHOLD = 10;        // 2/3 threshold for signatures
constexpr uint64_t MIN_PEGIN_AMOUNT = 10000;       // 0.0001 BTC minimum (in satoshis)
constexpr uint64_t MAX_PEGIN_AMOUNT = 10000000000; // 100 BTC maximum (in satoshis)
constexpr uint64_t BRIDGE_FEE_SATOSHIS = 1000;     // 0.00001 BTC bridge fee
constexpr uint64_t EMERGENCY_TIMEOUT_BLOCKS = 1008; // ~1 week timeout for emergency recovery

/**
 * Bitcoin Transaction Types
 */
enum class TxType {
    STANDARD,
    MULTISIG,
    SEGWIT,
    TAPROOT
};

/**
 * Peg Status States
 */
enum class PegStatus {
    INITIATED,           // Peg request submitted
    PENDING_BTC_CONF,    // Waiting for Bitcoin confirmations
    BTC_CONFIRMED,       // Bitcoin tx confirmed, pending AILEE mint
    MINTED,              // AILEE tokens minted successfully
    BURN_INITIATED,      // Peg-out burn started
    PENDING_PEGOUT,      // Waiting for federation signatures
    COMPLETED,           // Peg completed successfully
    FAILED,              // Peg failed
    EMERGENCY_RECOVERY   // In emergency recovery mode
};

/**
 * SPV Proof Structure
 * Simplified Payment Verification for trustless validation
 */
class SPVProof {
public:
    struct ProofData {
        std::string txId;
        uint32_t voutIndex;
        std::vector<uint8_t> transaction;
        std::vector<std::vector<uint8_t>> merkleProof;
        std::string blockHash;
        uint64_t blockHeight;
        uint32_t blockIndex;
    };

    SPVProof(const ProofData& data) : data_(data) {}

    /**
     * Verify the SPV proof against a block header
     */
    static bool verify(
        const ProofData& proof,
        const std::vector<uint8_t>& blockHeader
    ) {
        // Extract merkle root from block header (bytes 36-68)
        if (blockHeader.size() < 80) return false;
        
        std::vector<uint8_t> merkleRoot(blockHeader.begin() + 36, 
                                       blockHeader.begin() + 68);
        
        // Calculate tx hash
        auto txHash = doubleSHA256(proof.transaction);
        
        // Verify merkle proof
        std::vector<uint8_t> currentHash = txHash;
        
        for (const auto& sibling : proof.merkleProof) {
            // Concatenate and hash
            std::vector<uint8_t> combined;
            
            // Determine order based on sibling position
            if (currentHash < sibling) {
                combined.insert(combined.end(), currentHash.begin(), currentHash.end());
                combined.insert(combined.end(), sibling.begin(), sibling.end());
            } else {
                combined.insert(combined.end(), sibling.begin(), sibling.end());
                combined.insert(combined.end(), currentHash.begin(), currentHash.end());
            }
            
            currentHash = doubleSHA256(combined);
        }
        
        return currentHash == merkleRoot;
    }

    const ProofData& getData() const { return data_; }

private:
    ProofData data_;

    static std::vector<uint8_t> doubleSHA256(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> hash1(SHA256_DIGEST_LENGTH);
        SHA256(data.data(), data.size(), hash1.data());
        
        std::vector<uint8_t> hash2(SHA256_DIGEST_LENGTH);
        SHA256(hash1.data(), hash1.size(), hash2.data());
        
        return hash2;
    }
};

/**
 * Federation Signer
 * Member of the multi-sig federation managing the peg
 */
class FederationSigner {
public:
    struct SignerData {
        std::string signerId;
        std::string publicKey;
        std::string btcAddress;
        uint64_t stake;
        uint64_t reputationScore;
        uint64_t signatureCount;
        uint64_t missedSignatures;
        bool active;
        uint64_t joinedTime;
    };

    FederationSigner(
        const std::string& id,
        const std::string& pubKey,
        const std::string& btcAddr,
        uint64_t stake
    ) {
        data_.signerId = id;
        data_.publicKey = pubKey;
        data_.btcAddress = btcAddr;
        data_.stake = stake;
        data_.reputationScore = 100;
        data_.signatureCount = 0;
        data_.missedSignatures = 0;
        data_.active = true;
        data_.joinedTime = getCurrentTimestamp();
    }

    void recordSignature() {
        data_.signatureCount++;
        if (data_.reputationScore < 100) {
            data_.reputationScore = std::min<uint64_t>(100ULL, data_.reputationScore + 1);
        }
    }

    void recordMissedSignature() {
        data_.missedSignatures++;
        data_.reputationScore = std::max<uint64_t>(0ULL, data_.reputationScore - 5);
        if (data_.missedSignatures >= 10) {
            data_.active = false;
        }
    }

    double getResponseRate() const {
        uint64_t total = data_.signatureCount + data_.missedSignatures;
        if (total == 0) return 1.0;
        return static_cast<double>(data_.signatureCount) / static_cast<double>(total);
    }

    nlohmann::json to_json() const {
        nlohmann::json j = nlohmann::json();
        j["signerId"] = data_.signerId;
        j["publicKey"] = data_.publicKey;
        j["btcAddress"] = data_.btcAddress;
        j["stake"] = static_cast<double>(data_.stake);
        j["reputationScore"] = static_cast<double>(data_.reputationScore);
        j["signatureCount"] = static_cast<double>(data_.signatureCount);
        j["missedSignatures"] = static_cast<double>(data_.missedSignatures);
        j["active"] = data_.active;
        j["joinedTime"] = static_cast<double>(data_.joinedTime);
        return j;
    }

    void from_json(const nlohmann::json& j) {
        data_.signerId = j.value("signerId", "");
        data_.publicKey = j.value("publicKey", "");
        data_.btcAddress = j.value("btcAddress", "");
        data_.stake = j.value("stake", 0ULL);
        data_.reputationScore = j.value("reputationScore", 100ULL);
        data_.signatureCount = j.value("signatureCount", 0ULL);
        data_.missedSignatures = j.value("missedSignatures", 0ULL);
        data_.active = j.value("active", true);
        data_.joinedTime = j.value("joinedTime", 0ULL);
    }

    const SignerData& getData() const { return data_; }
    bool isActive() const { return data_.active; }
    std::string getId() const { return data_.signerId; }

private:
    SignerData data_;

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }
};

/**
 * Federation Manager
 * Coordinates multi-signature operations
 */
class FederationManager {
public:
    FederationManager() : requiredSignatures_(FEDERATION_THRESHOLD) {}

    bool addSigner(std::shared_ptr<FederationSigner> signer) {
        if (signers_.size() >= FEDERATION_SIZE) return false;
        
        signers_[signer->getId()] = signer;
        return true;
    }

    bool removeSigner(const std::string& signerId) {
        return signers_.erase(signerId) > 0;
    }

    std::vector<std::string> getActiveSigners() const {
        std::vector<std::string> active;
        for (const auto& pair : signers_) {
            if (pair.second->isActive()) {
                active.push_back(pair.first);
            }
        }
        return active;
    }

    size_t getActiveSignerCount() const {
        return getActiveSigners().size();
    }

    std::shared_ptr<FederationSigner> getSigner(const std::string& id) {
        auto it = signers_.find(id);
        return (it != signers_.end()) ? it->second : nullptr;
    }

    bool hasQuorum() const {
        return getActiveSignerCount() >= requiredSignatures_;
    }

    size_t getRequiredSignatures() const { return requiredSignatures_; }

private:
    std::map<std::string, std::shared_ptr<FederationSigner>> signers_;
    size_t requiredSignatures_;
};

/**
 * Peg-In Transaction
 * Represents a BTC → AILEE transfer
 */
class PegInTransaction {
public:
    struct PegInData {
        std::string pegId;
        std::string btcTxId;
        uint32_t btcVout;
        uint64_t btcAmount;           // In satoshis
        std::string btcSourceAddress;
        std::string aileeDestAddress;
        uint64_t btcBlockHeight;
        uint64_t btcConfirmations;
        uint64_t aileeMintAmount;     // After fees
        uint64_t initiatedTime;
        uint64_t completedTime;
        PegStatus status;
        SPVProof::ProofData spvProof;
    };

    PegInTransaction(
        const std::string& txId,
        uint32_t vout,
        uint64_t amount,
        const std::string& btcSource,
        const std::string& aileeDest
    ) {
        data_.pegId = generatePegId(txId, vout);
        data_.btcTxId = txId;
        data_.btcVout = vout;
        data_.btcAmount = amount;
        data_.btcSourceAddress = btcSource;
        data_.aileeDestAddress = aileeDest;
        data_.btcBlockHeight = 0;
        data_.btcConfirmations = 0;
        data_.aileeMintAmount = calculateMintAmount(amount);
        data_.initiatedTime = getCurrentTimestamp();
        data_.completedTime = 0;
        data_.status = PegStatus::INITIATED;
    }

    bool validateAmount() const {
        return data_.btcAmount >= MIN_PEGIN_AMOUNT &&
               data_.btcAmount <= MAX_PEGIN_AMOUNT;
    }

    bool updateConfirmations(uint64_t blockHeight, uint64_t currentHeight) {
        data_.btcBlockHeight = blockHeight;
        data_.btcConfirmations = (currentHeight > blockHeight) 
                                ? (currentHeight - blockHeight) 
                                : 0;
        
        if (data_.btcConfirmations >= MIN_CONFIRMATIONS_PEGIN) {
            if (data_.status == PegStatus::PENDING_BTC_CONF) {
                data_.status = PegStatus::BTC_CONFIRMED;
                return true;
            }
        }
        
        return false;
    }

    bool attachSPVProof(const SPVProof::ProofData& proof) {
        data_.spvProof = proof;
        data_.status = PegStatus::PENDING_BTC_CONF;
        return true;
    }

    bool completeMint() {
        if (data_.status != PegStatus::BTC_CONFIRMED) return false;
        
        data_.status = PegStatus::MINTED;
        data_.completedTime = getCurrentTimestamp();
        return true;
    }

    const PegInData& getData() const { return data_; }
    PegStatus getStatus() const { return data_.status; }

private:
    PegInData data_;

    static uint64_t calculateMintAmount(uint64_t btcAmount) {
        // Subtract bridge fee
        return (btcAmount > BRIDGE_FEE_SATOSHIS) 
               ? (btcAmount - BRIDGE_FEE_SATOSHIS) 
               : 0;
    }

    static std::string generatePegId(const std::string& txId, uint32_t vout) {
        std::string combined = txId + std::to_string(vout);
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(combined.data()),
               combined.size(), hash);
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            snprintf(hexStr + (i * 2), 3, "%02x", hash[i]);
        }
        return std::string(hexStr, 64);
    }

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }
};

/**
 * Peg-Out Transaction
 * Represents an AILEE → BTC transfer
 */
class PegOutTransaction {
public:
    struct PegOutData {
        std::string pegId;
        std::string aileeSourceAddress;
        std::string btcDestAddress;
        uint64_t aileeBurnAmount;
        uint64_t btcReleaseAmount;    // After fees
        uint64_t aileeBurnTxHeight;
        uint64_t aileeConfirmations;
        std::string btcReleaseTxId;
        std::string anchorCommitmentHash;
        uint64_t initiatedTime;
        uint64_t completedTime;
        PegStatus status;
        std::map<std::string, std::vector<uint8_t>> signatures;
    };

    PegOutTransaction(
        const std::string& aileeSource,
        const std::string& btcDest,
        uint64_t amount,
        const std::string& anchorCommitmentHash
    ) {
        data_.pegId = generatePegId(aileeSource, amount);
        data_.aileeSourceAddress = aileeSource;
        data_.btcDestAddress = btcDest;
        data_.aileeBurnAmount = amount;
        data_.btcReleaseAmount = calculateReleaseAmount(amount);
        data_.aileeBurnTxHeight = 0;
        data_.aileeConfirmations = 0;
        data_.initiatedTime = getCurrentTimestamp();
        data_.completedTime = 0;
        data_.status = PegStatus::BURN_INITIATED;
        data_.anchorCommitmentHash = anchorCommitmentHash;
    }

    bool updateConfirmations(uint64_t burnHeight, uint64_t currentHeight) {
        data_.aileeBurnTxHeight = burnHeight;
        data_.aileeConfirmations = (currentHeight > burnHeight)
                                  ? (currentHeight - burnHeight)
                                  : 0;
        
        if (data_.aileeConfirmations >= MIN_CONFIRMATIONS_PEGOUT) {
            if (data_.status == PegStatus::BURN_INITIATED) {
                data_.status = PegStatus::PENDING_PEGOUT;
                return true;
            }
        }
        
        return false;
    }

    bool addSignature(const std::string& signerId, 
                     const std::vector<uint8_t>& signature) {
        if (data_.status != PegStatus::PENDING_PEGOUT) return false;
        
        data_.signatures[signerId] = signature;
        return true;
    }

    bool hasRequiredSignatures(size_t threshold) const {
        return data_.signatures.size() >= threshold;
    }

    bool completeRelease(const std::string& btcTxId) {
        if (data_.status != PegStatus::PENDING_PEGOUT) return false;
        
        data_.btcReleaseTxId = btcTxId;
        data_.status = PegStatus::COMPLETED;
        data_.completedTime = getCurrentTimestamp();
        return true;
    }

    nlohmann::json to_json() const {
        nlohmann::json sigs = nlohmann::json();
        for (const auto& [k, v] : data_.signatures) {
            std::string sigHex;
            sigHex.reserve(v.size() * 2);
            for (uint8_t byte : v) {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02x", byte);
                sigHex += buf;
            }
            sigs[k] = sigHex;
        }

        nlohmann::json j = nlohmann::json();
        j["pegId"] = data_.pegId;
        j["aileeSourceAddress"] = data_.aileeSourceAddress;
        j["btcDestAddress"] = data_.btcDestAddress;
        j["aileeBurnAmount"] = static_cast<double>(data_.aileeBurnAmount);
        j["btcReleaseAmount"] = static_cast<double>(data_.btcReleaseAmount);
        j["aileeBurnTxHeight"] = static_cast<double>(data_.aileeBurnTxHeight);
        j["aileeConfirmations"] = static_cast<double>(data_.aileeConfirmations);
        j["btcReleaseTxId"] = data_.btcReleaseTxId;
        j["anchorCommitmentHash"] = data_.anchorCommitmentHash;
        j["initiatedTime"] = static_cast<double>(data_.initiatedTime);
        j["completedTime"] = static_cast<double>(data_.completedTime);
        j["status"] = static_cast<int>(data_.status);
        j["signatures"] = sigs;
        return j;
    }

    void from_json(const nlohmann::json& j) {
        data_.pegId = j.value("pegId", "");
        data_.aileeSourceAddress = j.value("aileeSourceAddress", "");
        data_.btcDestAddress = j.value("btcDestAddress", "");
        data_.aileeBurnAmount = j.value("aileeBurnAmount", 0ULL);
        data_.btcReleaseAmount = j.value("btcReleaseAmount", 0ULL);
        data_.aileeBurnTxHeight = j.value("aileeBurnTxHeight", 0ULL);
        data_.aileeConfirmations = j.value("aileeConfirmations", 0ULL);
        data_.btcReleaseTxId = j.value("btcReleaseTxId", "");
        data_.anchorCommitmentHash = j.value("anchorCommitmentHash", "");
        data_.initiatedTime = j.value("initiatedTime", 0ULL);
        data_.completedTime = j.value("completedTime", 0ULL);
        data_.status = static_cast<PegStatus>(j.value("status", 0));

        if (j.contains("signatures")) {
            for (auto it = j["signatures"].begin(); it != j["signatures"].end(); ++it) {
                std::string sigHex = it.value().get<std::string>();
                std::vector<uint8_t> sigBytes;
                sigBytes.reserve(sigHex.length() / 2);
                for (size_t i = 0; i < sigHex.length(); i += 2) {
                    std::string byteString = sigHex.substr(i, 2);
                    sigBytes.push_back(static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16)));
                }
                data_.signatures[it.key()] = sigBytes;
            }
        }
    }

    const PegOutData& getData() const { return data_; }
    PegStatus getStatus() const { return data_.status; }

private:
    PegOutData data_;

    static uint64_t calculateReleaseAmount(uint64_t aileeAmount) {
        // Subtract bridge fee
        return (aileeAmount > BRIDGE_FEE_SATOSHIS)
               ? (aileeAmount - BRIDGE_FEE_SATOSHIS)
               : 0;
    }

    static std::string generatePegId(const std::string& addr, uint64_t amount) {
        std::string combined = addr + std::to_string(amount) + 
                              std::to_string(getCurrentTimestamp());
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(combined.data()),
               combined.size(), hash);
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            snprintf(hexStr + (i * 2), 3, "%02x", hash[i]);
        }
        return std::string(hexStr, 64);
    }

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }
};

/**
 * Atomic Swap Manager
 * Trustless peer-to-peer exchanges using HTLC
 */
class AtomicSwap {
public:
    struct SwapData {
        std::string swapId;
        std::string partyA;
        std::string partyB;
        uint64_t amountA;
        uint64_t amountB;
        std::string hashLock;         // SHA256 hash of secret
        uint64_t timelock;            // Unix timestamp
        bool claimedByA;
        bool claimedByB;
        bool refundedA;
        bool refundedB;
    };

    AtomicSwap(
        const std::string& pA,
        const std::string& pB,
        uint64_t amtA,
        uint64_t amtB,
        const std::string& hash,
        uint64_t lockTime
    ) {
        data_.swapId = generateSwapId(pA, pB);
        data_.partyA = pA;
        data_.partyB = pB;
        data_.amountA = amtA;
        data_.amountB = amtB;
        data_.hashLock = hash;
        data_.timelock = lockTime;
        data_.claimedByA = false;
        data_.claimedByB = false;
        data_.refundedA = false;
        data_.refundedB = false;
    }

    bool claim(const std::string& party, const std::string& secret) {
        // Verify secret matches hash
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(secret.data()),
               secret.size(), hash);
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            snprintf(hexStr + (i * 2), 3, "%02x", hash[i]);
        }
        std::string secretHash(hexStr, 64);
        
        if (secretHash != data_.hashLock) return false;
        
        // Check timelock
        uint64_t now = getCurrentTimestamp();
        if (now >= data_.timelock) return false;
        
        // Mark as claimed
        if (party == data_.partyA && !data_.claimedByA) {
            data_.claimedByA = true;
            return true;
        } else if (party == data_.partyB && !data_.claimedByB) {
            data_.claimedByB = true;
            return true;
        }
        
        return false;
    }

    bool refund(const std::string& party) {
        uint64_t now = getCurrentTimestamp();
        if (now < data_.timelock) return false;
        
        if (party == data_.partyA && !data_.claimedByA && !data_.refundedA) {
            data_.refundedA = true;
            return true;
        } else if (party == data_.partyB && !data_.claimedByB && !data_.refundedB) {
            data_.refundedB = true;
            return true;
        }
        
        return false;
    }

    bool isComplete() const {
        return data_.claimedByA && data_.claimedByB;
    }

    const SwapData& getData() const { return data_; }

private:
    SwapData data_;

    static std::string generateSwapId(const std::string& pA, const std::string& pB) {
        std::string combined = pA + pB + std::to_string(getCurrentTimestamp());
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(combined.data()),
               combined.size(), hash);
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            snprintf(hexStr + (i * 2), 3, "%02x", hash[i]);
        }
        return std::string(hexStr, 64);
    }

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }
};

/**
 * Bridge Statistics & Monitoring
 */
class BridgeStatistics {
public:
    struct Stats {
        uint64_t totalPegins;
        uint64_t totalPegouts;
        uint64_t totalVolumeBTC;       // In satoshis
        uint64_t currentLockedBTC;     // Currently locked on Bitcoin mainnet
        uint64_t currentMintedAILEE;   // Currently minted on AILEE
        uint64_t totalFeesCollected;
        double averagePeginTime;       // In seconds
        double averagePegoutTime;      // In seconds
        size_t activeFederationSigners;
        double federationHealthScore;  // 0.0 to 1.0
    };

    BridgeStatistics() {
        stats_.totalPegins = 0;
        stats_.totalPegouts = 0;
        stats_.totalVolumeBTC = 0;
        stats_.currentLockedBTC = 0;
        stats_.currentMintedAILEE = 0;
        stats_.totalFeesCollected = 0;
        stats_.averagePeginTime = 0.0;
        stats_.averagePegoutTime = 0.0;
        stats_.activeFederationSigners = 0;
        stats_.federationHealthScore = 1.0;
    }

    void recordPegin(uint64_t amount, uint64_t duration) {
        stats_.totalPegins++;
        stats_.totalVolumeBTC += amount;
        stats_.currentLockedBTC += amount;
        stats_.currentMintedAILEE += (amount - BRIDGE_FEE_SATOSHIS);
        stats_.totalFeesCollected += BRIDGE_FEE_SATOSHIS;
        
        // Update moving average
        stats_.averagePeginTime = 
            (stats_.averagePeginTime * (stats_.totalPegins - 1) + duration) / 
            stats_.totalPegins;
    }

    void recordPegout(uint64_t amount, uint64_t duration) {
        stats_.totalPegouts++;
        stats_.totalVolumeBTC += amount;
        stats_.currentLockedBTC -= amount;
        stats_.currentMintedAILEE -= amount;
        stats_.totalFeesCollected += BRIDGE_FEE_SATOSHIS;
        
        // Update moving average
        stats_.averagePegoutTime = 
            (stats_.averagePegoutTime * (stats_.totalPegouts - 1) + duration) / 
            stats_.totalPegouts;
    }

    void updateFederationHealth(size_t activeSigners, double healthScore) {
        stats_.activeFederationSigners = activeSigners;
        stats_.federationHealthScore = healthScore;
    }

    const Stats& getStats() const { return stats_; }

    double getCollateralizationRatio() const {
        if (stats_.currentMintedAILEE == 0) return 0.0;
        return static_cast<double>(stats_.currentLockedBTC) / 
               static_cast<double>(stats_.currentMintedAILEE);
    }

private:
    Stats stats_;
};

/**
 * Sidechain Bridge Manager
 * Main orchestrator for all bridge operations
 */
class SidechainBridge {
public:
    SidechainBridge(ailee::storage::PersistentStorage* storage = nullptr)
        : federation_(std::make_unique<FederationManager>()),
          statistics_(std::make_unique<BridgeStatistics>()),
          emergencyMode_(false),
          storage_(storage) {
        recoverState();
    }

    void recoverState() {
        if (!storage_) return;

        auto idxOpt = storage_->get("bridge/federation/signer_index");
        if (idxOpt) {
            try {
                auto arr = nlohmann::json::parse(*idxOpt);
                for (const auto& id_json : arr) {
                    std::string id = id_json.get<std::string>();
                    auto signerOpt = storage_->get("bridge/federation/signer/" + id);
                    if (signerOpt) {
                        auto signer_j = nlohmann::json::parse(*signerOpt);
                        auto signer = std::make_shared<FederationSigner>("", "", "", 0);
                        signer->from_json(signer_j);
                        federation_->addSigner(signer);
                    }
                }
            } catch (...) {}
        }

        auto pegoutIdxOpt = storage_->get("bridge/pegout_index");
        if (pegoutIdxOpt) {
            try {
                auto arr = nlohmann::json::parse(*pegoutIdxOpt);
                for (const auto& id_json : arr) {
                    std::string id = id_json.get<std::string>();
                    auto pegoutOpt = storage_->get("bridge/pegout/" + id);
                    if (pegoutOpt) {
                        auto pegout_j = nlohmann::json::parse(*pegoutOpt);
                        auto pegout = std::make_shared<PegOutTransaction>("", "", 0, "");
                        pegout->from_json(pegout_j);
                        pegouts_[id] = pegout;
                    }
                }
            } catch (...) {}
        }
    }


void persistSigners(const std::vector<std::string>& signerIds) {
        if (!storage_) return;
        std::vector<ailee::storage::PersistentStorage::BatchOp> ops;
        ops.reserve(signerIds.size() + 1);
        for (const auto& id : signerIds) {
            auto signer = federation_->getSigner(id);
            if (signer) {
                ailee::storage::PersistentStorage::BatchOp op;
                op.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
                op.key = "bridge/federation/signer/" + id;
                op.value = signer->to_json().dump();
                ops.push_back(op);
            }
        }

        auto active_signers = federation_->getActiveSigners();
        nlohmann::json arr = nlohmann::json::array_t{};
        for (const auto& s : active_signers) {
            arr.push_back(s);
        }
        ailee::storage::PersistentStorage::BatchOp idxOp;
        idxOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
        idxOp.key = "bridge/federation/signer_index";
        idxOp.value = arr.dump();
        ops.push_back(idxOp);

        if (!ops.empty()) {
            storage_->executeBatch(ops);
        }
    }

    // Federation management
    bool addFederationSigner(
        const std::string& id,
        const std::string& pubKey,
        const std::string& btcAddr,
        uint64_t stake
    ) {
        auto signer = std::make_shared<FederationSigner>(id, pubKey, btcAddr, stake);
        if (storage_) {
            auto signerOpt = storage_->get("bridge/federation/signer/" + id);
            if (signerOpt) {
                try {
                    auto signer_j = nlohmann::json::parse(*signerOpt);
                    signer->from_json(signer_j);
                } catch (...) {}
            }
        }
        if (federation_->addSigner(signer)) {
            persistSigners({id});
            return true;
        }
        return false;
    }

    // Peg-in operations
    std::string initiatePegIn(
        const std::string& btcTxId,
        uint32_t vout,
        uint64_t amount,
        const std::string& btcSource,
        const std::string& aileeDest
    ) {
        auto pegin = std::make_shared<PegInTransaction>(
            btcTxId, vout, amount, btcSource, aileeDest
        );

        if (!pegin->validateAmount()) return "";

        std::string pegId = pegin->getData().pegId;
        pegins_[pegId] = pegin;

        return pegId;
    }

    bool submitSPVProof(
        const std::string& pegId,
        const SPVProof::ProofData& proof,
        const std::vector<uint8_t>& blockHeader
    ) {
        auto it = pegins_.find(pegId);
        if (it == pegins_.end()) return false;

        // Verify SPV proof
        if (!SPVProof::verify(proof, blockHeader)) return false;

        return it->second->attachSPVProof(proof);
    }

    bool updatePegInConfirmations(
        const std::string& pegId,
        uint64_t btcBlockHeight,
        uint64_t currentBtcHeight
    ) {
        auto it = pegins_.find(pegId);
        if (it == pegins_.end()) return false;

        return it->second->updateConfirmations(btcBlockHeight, currentBtcHeight);
    }

    bool completePegInMint(const std::string& pegId) {
        auto it = pegins_.find(pegId);
        if (it == pegins_.end()) return false;

        if (it->second->completeMint()) {
            auto& data = it->second->getData();
            uint64_t duration = data.completedTime - data.initiatedTime;
            statistics_->recordPegin(data.btcAmount, duration);
            return true;
        }

        return false;
    }

    // Peg-out operations
    std::string initiatePegOut(
        const std::string& aileeSource,
        const std::string& btcDest,
        uint64_t amount,
        const std::string& anchorCommitmentHash
    ) {
        if (anchorCommitmentHash.empty()) return "";
        if (anchorCommitments_.find(anchorCommitmentHash) == anchorCommitments_.end()) return "";

        auto pegout = std::make_shared<PegOutTransaction>(
            aileeSource, btcDest, amount, anchorCommitmentHash
        );

        std::string pegId = pegout->getData().pegId;
        pegouts_[pegId] = pegout;

        return pegId;
    }

    bool updatePegOutConfirmations(
        const std::string& pegId,
        uint64_t burnHeight,
        uint64_t currentHeight
    ) {
        auto it = pegouts_.find(pegId);
        if (it == pegouts_.end()) return false;

        return it->second->updateConfirmations(burnHeight, currentHeight);
    }

bool verifyPegOutSignature(
        const std::string& pegId,
        const std::string& aileeSourceAddress,
        const std::string& btcDestAddress,
        uint64_t aileeBurnAmount,
        uint64_t btcReleaseAmount,
        const std::string& anchorCommitmentHash,
        const std::string& signerPubKeyHex,
        const std::vector<uint8_t>& signature
    ) const {
        if (signature.size() < 64) return false;

        std::string payload = pegId + "|" + aileeSourceAddress + "|" + btcDestAddress + "|" +
                              std::to_string(aileeBurnAmount) + "|" + std::to_string(btcReleaseAmount) + "|" +
                              anchorCommitmentHash;

        unsigned char hash1[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), hash1);
        unsigned char hash2[SHA256_DIGEST_LENGTH];
        SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);

        secp256k1_context* ctx = getBridgeSecp256k1VerifyContext();

        std::vector<uint8_t> pubKeyBytes;
        pubKeyBytes.reserve(signerPubKeyHex.length() / 2);
        for (size_t i = 0; i < signerPubKeyHex.length(); i += 2) {
            std::string byteString = signerPubKeyHex.substr(i, 2);
            pubKeyBytes.push_back(static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16)));
        }

        secp256k1_pubkey pubkey;
        if (secp256k1_ec_pubkey_parse(ctx, &pubkey, pubKeyBytes.data(), pubKeyBytes.size()) != 1) {
            return false;
        }

        secp256k1_ecdsa_signature sig;
        bool parsed = false;

        if (signature.size() == 64) {
            if (secp256k1_ecdsa_signature_parse_compact(ctx, &sig, signature.data()) == 1) {
                parsed = true;
            }
        }

        if (!parsed) {
            if (secp256k1_ecdsa_signature_parse_der(ctx, &sig, signature.data(), signature.size()) == 1) {
                parsed = true;
            }
        }

        if (!parsed) {
            return false;
        }

        secp256k1_ecdsa_signature normalized_sig;
        secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &sig);

        return secp256k1_ecdsa_verify(ctx, &normalized_sig, hash2, &pubkey) == 1;
    }

    bool signPegOut(
        const std::string& pegId,
        const std::string& signerId,
        const std::vector<uint8_t>& signature
    ) {
        auto pegoutIt = pegouts_.find(pegId);
        if (pegoutIt == pegouts_.end()) return false;
        if (!isPegOutAnchorAuthorized(*pegoutIt->second)) return false;

        auto signer = federation_->getSigner(signerId);
        if (!signer || !signer->isActive()) return false;

        const auto& data = pegoutIt->second->getData();
        if (!verifyPegOutSignature(pegId, data.aileeSourceAddress, data.btcDestAddress,
                                   data.aileeBurnAmount, data.btcReleaseAmount,
                                   data.anchorCommitmentHash, signer->getData().publicKey, signature)) {
            return false;
        }

        if (pegoutIt->second->addSignature(signerId, signature)) {
            signer->recordSignature();
            return true;
        }

        return false;
    }

    bool completePegOut(const std::string& pegId, const std::string& btcTxId) {
        auto it = pegouts_.find(pegId);
        if (it == pegouts_.end()) return false;
        if (!isPegOutAnchorAuthorized(*it->second)) return false;

        size_t threshold = federation_->getRequiredSignatures();
        if (!it->second->hasRequiredSignatures(threshold)) return false;

        if (it->second->completeRelease(btcTxId)) {
            auto& data = it->second->getData();
            uint64_t duration = data.completedTime - data.initiatedTime;
            statistics_->recordPegout(data.aileeBurnAmount, duration);

            if (storage_) {
                std::vector<ailee::storage::PersistentStorage::BatchOp> ops;

                ailee::storage::PersistentStorage::BatchOp pegOp;
                pegOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
                pegOp.key = "bridge/pegout/" + pegId;
                pegOp.value = it->second->to_json().dump();
                ops.push_back(pegOp);

                nlohmann::json arr = nlohmann::json::array_t{};
                for (const auto& [id, _] : pegouts_) {
                    arr.push_back(id);
                }
                ailee::storage::PersistentStorage::BatchOp idxOp;
                idxOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
                idxOp.key = "bridge/pegout_index";
                idxOp.value = arr.dump();
                ops.push_back(idxOp);

                for (const auto& [signerId, signature] : data.signatures) {
                    auto signer = federation_->getSigner(signerId);
                    if (signer) {
                        ailee::storage::PersistentStorage::BatchOp sigOp;
                        sigOp.type = ailee::storage::PersistentStorage::BatchOpType::PUT;
                        sigOp.key = "bridge/federation/signer/" + signerId;
                        sigOp.value = signer->to_json().dump();
                        ops.push_back(sigOp);
                    }
                }

                storage_->executeBatch(ops);
            }

            return true;
        }

        return false;
    }

    // Atomic swap operations
    std::string createAtomicSwap(
        const std::string& partyA,
        const std::string& partyB,
        uint64_t amountA,
        uint64_t amountB,
        const std::string& hashLock,
        uint64_t timelockDuration
    ) {
        uint64_t timelock = getCurrentTimestamp() + timelockDuration;
        
        auto swap = std::make_shared<AtomicSwap>(
            partyA, partyB, amountA, amountB, hashLock, timelock
        );

        std::string swapId = swap->getData().swapId;
        atomicSwaps_[swapId] = swap;

        return swapId;
    }

    bool claimAtomicSwap(
        const std::string& swapId,
        const std::string& party,
        const std::string& secret
    ) {
        auto it = atomicSwaps_.find(swapId);
        if (it == atomicSwaps_.end()) return false;

        return it->second->claim(party, secret);
    }

    bool refundAtomicSwap(const std::string& swapId, const std::string& party) {
        auto it = atomicSwaps_.find(swapId);
        if (it == atomicSwaps_.end()) return false;

        return it->second->refund(party);
    }

    // Emergency operations
    bool activateEmergencyMode() {
        emergencyMode_ = true;
        return true;
    }

    bool deactivateEmergencyMode() {
        // Only deactivate if federation has quorum
        if (!federation_->hasQuorum()) return false;
        
        emergencyMode_ = false;
        return true;
    }

    bool isEmergencyMode() const { return emergencyMode_; }

    // Statistics and monitoring
    BridgeStatistics::Stats getStatistics() const {
        return statistics_->getStats();
    }

    double getCollateralizationRatio() const {
        return statistics_->getCollateralizationRatio();
    }

    size_t getActiveFederationSigners() const {
        return federation_->getActiveSignerCount();
    }

    bool isBridgeHealthy() const {
        // Check collateralization
        double collateral = getCollateralizationRatio();
        if (collateral < 0.95 || collateral > 1.05) return false;

        // Check federation quorum
        if (!federation_->hasQuorum()) return false;

        // Check emergency mode
        if (emergencyMode_) return false;

        return true;
    }

    // Accessors
    std::shared_ptr<PegInTransaction> getPegIn(const std::string& pegId) {
        auto it = pegins_.find(pegId);
        return (it != pegins_.end()) ? it->second : nullptr;
    }

    std::shared_ptr<PegOutTransaction> getPegOut(const std::string& pegId) {
        auto it = pegouts_.find(pegId);
        return (it != pegouts_.end()) ? it->second : nullptr;
    }

    FederationManager* getFederation() { return federation_.get(); }

    bool registerAnchorCommitment(const ailee::global_seven::AnchorCommitment& anchor,
                                  const std::string& expectedStateRoot) {
        if (anchor.l2StateRoot != expectedStateRoot) return false;
        auto normalize = [](std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        };
        if (normalize(ailee::zk::sha256Hex(anchor.payload)) != normalize(anchor.hash)) return false;
        anchorCommitments_[anchor.hash] = anchor;
        return true;
    }

    ailee::l2::BridgeSnapshot snapshotBridgeState() const {
        ailee::l2::BridgeSnapshot snapshot;
        snapshot.pegins.reserve(pegins_.size());
        for (const auto& [pegId, pegin] : pegins_) {
            const auto& data = pegin->getData();
            snapshot.pegins.push_back(ailee::l2::PegInSnapshot{
                pegId,
                data.btcTxId,
                data.btcVout,
                data.btcAmount,
                data.btcSourceAddress,
                data.aileeDestAddress,
                data.btcConfirmations,
                data.initiatedTime,
                data.completedTime,
                static_cast<int>(data.status)
            });
        }
        snapshot.pegouts.reserve(pegouts_.size());
        for (const auto& [pegId, pegout] : pegouts_) {
            const auto& data = pegout->getData();
            snapshot.pegouts.push_back(ailee::l2::PegOutSnapshot{
                pegId,
                data.aileeSourceAddress,
                data.btcDestAddress,
                data.aileeBurnAmount,
                data.btcReleaseAmount,
                data.initiatedTime,
                data.completedTime,
                static_cast<int>(data.status),
                data.anchorCommitmentHash
            });
        }
        return snapshot;
    }

private:
    std::unique_ptr<FederationManager> federation_;
    std::unique_ptr<BridgeStatistics> statistics_;
    std::map<std::string, std::shared_ptr<PegInTransaction>> pegins_;
    std::map<std::string, std::shared_ptr<PegOutTransaction>> pegouts_;
    std::map<std::string, std::shared_ptr<AtomicSwap>> atomicSwaps_;
    std::map<std::string, ailee::global_seven::AnchorCommitment> anchorCommitments_;
    bool emergencyMode_;
    ailee::storage::PersistentStorage* storage_;

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    bool isPegOutAnchorAuthorized(const PegOutTransaction& pegout) const {
        const auto& anchorHash = pegout.getData().anchorCommitmentHash;
        return !anchorHash.empty() && anchorCommitments_.find(anchorHash) != anchorCommitments_.end();
    }
};

/**
 * Bridge Security Monitor
 * Watches for anomalies and potential attacks
 */
class BridgeSecurityMonitor {
public:
    struct SecurityAlert {
        enum class AlertLevel {
            INFO,
            WARNING,
            CRITICAL
        };

        AlertLevel level;
        std::string message;
        uint64_t timestamp;
        std::string details;
    };

    void checkCollateralization(const SidechainBridge& bridge) {
        double ratio = bridge.getCollateralizationRatio();
        
        if (ratio < 0.90) {
            raiseAlert(
                SecurityAlert::AlertLevel::CRITICAL,
                "Under-collateralized bridge",
                "Collateralization ratio: " + std::to_string(ratio)
            );
        } else if (ratio < 0.95) {
            raiseAlert(
                SecurityAlert::AlertLevel::WARNING,
                "Low collateralization",
                "Collateralization ratio: " + std::to_string(ratio)
            );
        }
    }

    void checkFederationHealth(const SidechainBridge& bridge) {
        size_t activeSigners = bridge.getActiveFederationSigners();
        
        if (activeSigners < FEDERATION_THRESHOLD) {
            raiseAlert(
                SecurityAlert::AlertLevel::CRITICAL,
                "Federation below quorum",
                "Active signers: " + std::to_string(activeSigners)
            );
        } else if (activeSigners < FEDERATION_SIZE * 0.8) {
            raiseAlert(
                SecurityAlert::AlertLevel::WARNING,
                "Federation degraded",
                "Active signers: " + std::to_string(activeSigners)
            );
        }
    }

    void checkEmergencyMode(const SidechainBridge& bridge) {
        if (bridge.isEmergencyMode()) {
            raiseAlert(
                SecurityAlert::AlertLevel::CRITICAL,
                "Bridge in emergency mode",
                "Manual intervention required"
            );
        }
    }

    std::vector<SecurityAlert> getRecentAlerts(size_t count = 10) const {
        size_t start = (alerts_.size() > count) ? (alerts_.size() - count) : 0;
        return std::vector<SecurityAlert>(alerts_.begin() + start, alerts_.end());
    }

    size_t getAlertCount() const { return alerts_.size(); }

private:
    std::vector<SecurityAlert> alerts_;

    void raiseAlert(
        SecurityAlert::AlertLevel level,
        const std::string& message,
        const std::string& details
    ) {
        SecurityAlert alert;
        alert.level = level;
        alert.message = message;
        alert.details = details;
        alert.timestamp = getCurrentTimestamp();
        
        alerts_.push_back(alert);
    }

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }
};

} // namespace ailee

#endif // AILEE_SIDECHAIN_BRIDGE_H
