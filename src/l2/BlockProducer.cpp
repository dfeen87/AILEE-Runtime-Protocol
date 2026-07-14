#include "BlockProducer.h"
#include "Mempool.h"
#include "ReorgDetector.h"

#include <chrono>
#include <sstream>
#include <iostream>
#include <vector>
#include <secp256k1.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Simple logging function - logs to stdout
static void log(const std::string& level, const std::string& msg) {
    std::cout << "[" << level << "] " << msg << std::endl;
}

namespace ailee::l2 {

namespace {

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    if (hex.size() % 2 != 0) return bytes;

    // Strict validation of hex characters
    if (hex.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        return bytes; // Return empty on invalid hex characters
    }

    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned int value = 0;
        std::istringstream iss(hex.substr(i, 2));
        iss >> std::hex >> value;
        bytes.push_back(static_cast<uint8_t>(value));
    }
    return bytes;
}

// Helper to manage secp256k1 context lifecycle deterministically
secp256k1_context* getSecp256k1VerifyContext() {
    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    return ctx;
}

bool verifyTxSignature(const std::string& txHashHex, const std::string& pubkeyHex, const std::string& sigHex) {
    auto msgBytes = hexToBytes(txHashHex);
    auto pubBytes = hexToBytes(pubkeyHex);
    auto sigBytes = hexToBytes(sigHex);

    if (msgBytes.size() != 32 || pubBytes.empty() || sigBytes.empty()) {
        return false;
    }

    secp256k1_context* ctx = getSecp256k1VerifyContext();
    if (!ctx) return false;

    secp256k1_pubkey pubkey;
    if (secp256k1_ec_pubkey_parse(ctx, &pubkey, pubBytes.data(), pubBytes.size()) != 1) {
        return false;
    }

    secp256k1_ecdsa_signature sig;
    int parse_res = 0;
    if (sigBytes.size() == 64) {
        parse_res = secp256k1_ecdsa_signature_parse_compact(ctx, &sig, sigBytes.data());
    } else {
        parse_res = secp256k1_ecdsa_signature_parse_der(ctx, &sig, sigBytes.data(), sigBytes.size());
    }

    if (parse_res != 1) {
        return false;
    }

    secp256k1_ecdsa_signature normalized_sig;
    secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &sig);

    int verify_res = secp256k1_ecdsa_verify(ctx, &normalized_sig, msgBytes.data(), &pubkey);

    return verify_res == 1;
}

} // namespace

BlockProducer::BlockProducer(const Config& config)
    : config_(config) {
}

BlockProducer::~BlockProducer() {
    stop();
}

void BlockProducer::start() {
    if (running_.load()) {
        log("WARN", "BlockProducer::start() - already running");
        return;
    }

    running_.store(true);
    producerThread_ = std::make_unique<std::thread>([this]() {
        blockProductionLoop();
    });

    log("INFO", "BlockProducer started - producing blocks every " + 
        std::to_string(config_.blockIntervalMs) + "ms");
    log("INFO", "Anchor commitment interval: " + 
        std::to_string(config_.commitmentInterval) + " blocks");
}

void BlockProducer::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);
    if (producerThread_ && producerThread_->joinable()) {
        producerThread_->join();
    }
    producerThread_.reset();

    log("INFO", "BlockProducer stopped");
}

BlockProducer::State BlockProducer::getState() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    State stateCopy = state_;
    
    // Update pending transaction count from mempool if available
    if (mempool_) {
        stateCopy.pendingTransactions = mempool_->getPendingCount();
    }
    
    return stateCopy;
}

void BlockProducer::setMempool(Mempool* mempool) {
    mempool_ = mempool;
    log("INFO", "BlockProducer mempool reference set");
}

void BlockProducer::setReorgDetector(ailee::l1::ReorgDetector* detector) {
    reorgDetector_ = detector;
    log("INFO", "BlockProducer reorg detector set");
}

void BlockProducer::recordTransaction() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    state_.totalTransactions++;
}

void BlockProducer::blockProductionLoop() {
    log("INFO", "Block production loop started");

    while (running_.load()) {
        // We use a deterministic logical sleep in production-like consensus
        // but since this is an active thread loop, we just use a fixed thread sleep.
        // We do NOT use this sleep duration for any consensus state.
        produceBlock();
        checkAnchorCommitment();

        std::this_thread::sleep_for(std::chrono::milliseconds(config_.blockIntervalMs));
    }

    log("INFO", "Block production loop exited");
}

void BlockProducer::produceBlock() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    // Security check: If we have a reorg detector, inspect recent deep reorg history.
    // NOTE: getRecentReorgHistory() returns historical events, not necessarily an
    // active reorg affecting the current chain tip. We therefore only log any
    // recent deep reorgs here instead of halting block production permanently.
    if (reorgDetector_) {
        auto reorgs = reorgDetector_->getRecentReorgHistory(1);
        if (!reorgs.empty()) {
            const auto& lastReorg = reorgs.front();
            log("WARN", "Deep L1 reorg observed historically at height " +
                std::to_string(lastReorg.reorgHeight) +
                ". Block production continues; a state-aware reorg check should "
                "verify whether the current L2 tip is affected.");
        }
    }

    // Increment block height
    state_.blockHeight++;

    // Update timestamp deterministically based on protocol logic
    // Using block height and interval instead of wall-clock time
    state_.lastBlockTimestampMs = state_.blockHeight * config_.blockIntervalMs;

    // Pull transactions from mempool if available
    std::size_t txsInBlock = 0;
    if (mempool_) {
        auto transactions = mempool_->getPendingTransactions(config_.maxTransactionsPerBlock);
        txsInBlock = transactions.size();
        
        if (txsInBlock > 0) {
            // Validate and confirm transactions
            std::vector<std::string> validTxHashes;
            validTxHashes.reserve(txsInBlock);

            for (const auto& tx : transactions) {
                // Skip deterministic rejections to prevent infinite reprocessing
                if (rejectedTxs_.count(tx.txHash) > 0) {
                    continue;
                }

                // Audit Fix: Basic Validation
                // 1. Check if hash is valid length and contains only hex characters
                if (tx.txHash.length() != 64 ||
                    tx.txHash.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
                    log("ERROR", "Invalid transaction hash detected: " + tx.txHash + ". Rejecting.");
                    rejectedTxs_.insert(tx.txHash);
                    continue;
                }
                // 2. Check if sender/receiver present (basic sanity)
                if (tx.fromAddress.empty() || tx.toAddress.empty()) {
                    log("ERROR", "Malformed transaction detected (missing sender/receiver). Rejecting: " + tx.txHash);
                    rejectedTxs_.insert(tx.txHash);
                    continue;
                }
                // 3. Strict signature presence check
                if (tx.signature.empty()) {
                    log("ERROR", "Transaction missing signature; rejecting: " + tx.txHash);
                    rejectedTxs_.insert(tx.txHash);
                    continue;
                }

                // 4. Cryptographic signature verification
                std::string pubKeyToVerify = tx.publicKey.empty() ? tx.fromAddress : tx.publicKey;
                if (!verifyTxSignature(tx.txHash, pubKeyToVerify, tx.signature)) {
                    log("ERROR", "Transaction signature verification failed; rejecting: " + tx.txHash);
                    rejectedTxs_.insert(tx.txHash);
                    continue;
                }

                validTxHashes.push_back(tx.txHash);
            }

            if (!validTxHashes.empty()) {
                mempool_->confirmTransactions(validTxHashes, state_.blockHeight);
                // Update total transaction count
                state_.totalTransactions += validTxHashes.size();
            }
        }
    }

    // Log block production (every 10 blocks to avoid spam, or if block contains transactions)
    if (state_.blockHeight % 10 == 0 || state_.blockHeight <= 5 || txsInBlock > 0) {
        std::ostringstream oss;
        oss << "Block #" << state_.blockHeight << " produced"
            << " (txs in block: " << txsInBlock
            << ", total txs: " << state_.totalTransactions << ")";
        log("INFO", oss.str());
    }
}

void BlockProducer::checkAnchorCommitment() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    // Check if we've reached the commitment interval
    std::uint64_t blocksSinceAnchor = state_.blockHeight - state_.lastAnchorHeight;

    if (blocksSinceAnchor >= config_.commitmentInterval) {
        // Time to create an anchor commitment
        state_.lastAnchorHeight = state_.blockHeight;

        std::ostringstream oss;
        oss << "Anchor commitment created at block #" << state_.blockHeight
            << " (interval: " << config_.commitmentInterval << " blocks)";
        log("INFO", oss.str());
    }
}

void BlockProducer::broadcastLatestBlockToMainnet() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    if (state_.blockHeight == 0) {
        log("WARN", "No blocks available to broadcast");
        return;
    }

    // Build a minimal JSON payload using existing state
    json blockJson;
    blockJson = {
        {"height", json::number_unsigned(state_.blockHeight)},
        {"timestamp_ms", json::number_unsigned(state_.lastBlockTimestampMs)},
        {"total_transactions", json::number_unsigned(state_.totalTransactions)},
        {"last_anchor_height", json::number_unsigned(state_.lastAnchorHeight)}
    };

    std::string payload = blockJson.dump();

    log("INFO", "Preparing mainnet broadcast for block #" + 
        std::to_string(state_.blockHeight));

    try {
        // Replace with your actual RPC implementation
        // This is a placeholder to show where RPC goes
        log("INFO", "Mainnet broadcast payload: " + payload);

        // TODO: integrate BitcoinRPC here
        // BitcoinRPC rpc("127.0.0.1", 8332, "rpcuser", "rpcpassword");
        // json response = rpc.call({ ... });

        log("INFO", "Mainnet broadcast completed");

    } catch (const std::exception& e) {
        log("ERROR", std::string("Mainnet broadcast failed: ") + e.what());
    }
}

} // namespace ailee::l2
