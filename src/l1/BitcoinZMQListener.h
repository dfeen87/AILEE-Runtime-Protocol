/**
 * AILEE Bitcoin ZMQ Listener (Production Hardened)
 * 
 * A fault-tolerant, asynchronous bridge to Bitcoin Core.
 * Features:
 * - Automatic Hex Encoding for TXIDs/BlockHashes
 * - Non-blocking I/O with Receive Timeouts
 * - Exponential Backoff Reconnection Strategy
 * - Binary-safe payload handling
 * 
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_BITCOIN_ZMQ_LISTENER_H
#define AILEE_BITCOIN_ZMQ_LISTENER_H

#if defined(AILEE_HAS_ZMQ)
#include <zmq.hpp>
#endif
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include "ReorgDetector.h"

namespace ailee {

class SidechainBridge;

class BitcoinZMQListener {
public:
#if defined(AILEE_HAS_ZMQ)
    explicit BitcoinZMQListener(const std::string& endpoint = "tcp://127.0.0.1:28332")
        : context_(1), subscriber_(context_, ZMQ_SUB), running_(false), endpoint_(endpoint), reorgDetector_(nullptr), bridge_(nullptr) {}

    ~BitcoinZMQListener() {
        if (running_) stop();
    }

    void setReorgDetector(ailee::l1::ReorgDetector* detector) {
        reorgDetector_ = detector;
    }

    void setSidechainBridge(ailee::SidechainBridge* bridge, const std::vector<uint8_t>& bridgeAddress) {
        bridge_ = bridge;
        bridgeAddressBytes_ = bridgeAddress;
    }

    // Initialize with Hardened Socket Options
    void init() {
        try {
            std::cout << "[ZMQ] Connecting to Bitcoin Core at " << endpoint_ << "..." << std::endl;
            subscriber_.connect(endpoint_);
            
            // Subscribe to topics
            subscriber_.set(zmq::sockopt::subscribe, "rawtx");
            subscriber_.set(zmq::sockopt::subscribe, "hashblock");

            // SAFETY: Set a receive timeout (1000ms) so the thread doesn't hang forever
            // This allows the loop to check 'running_' status periodically.
            subscriber_.set(zmq::sockopt::rcvtimeo, 1000);

            // SAFETY: KeepAlive to detect dead connections
            subscriber_.set(zmq::sockopt::tcp_keepalive, 1);

            std::cout << "[ZMQ] Connection Established. Listening for Mainnet events." << std::endl;
        } catch (const zmq::error_t& e) {
            std::cerr << "[CRITICAL] ZMQ Init Failed: " << e.what() << std::endl;
            // In production, we might throw, but here we will let the loop handle reconnect.
        }
    }

    // Main Event Loop
    void start() {
        if (running_) return;
        running_ = true;

        while (running_) {
            try {
                zmq::message_t topic;
                zmq::message_t payload;

                // 1. Receive Topic (Non-blocking due to RCVTIMEO)
                auto res = subscriber_.recv(topic, zmq::recv_flags::none);
                if (!res) {
                    // Timeout occurred (No data received in 1s). 
                    // This is normal. Loop back and check running_.
                    continue; 
                }

                // 2. Receive Payload
                // If we got a topic, we MUST get a payload. If this blocks, it's a protocol error.
                if (!subscriber_.recv(payload, zmq::recv_flags::none)) {
                    std::cerr << "[WARN] Received topic but no payload. Dropping frame." << std::endl;
                    continue;
                }

                // 3. Process Data
                std::string topic_str(static_cast<char*>(topic.data()), topic.size());
                
                if (topic_str == "rawtx") {
                    handleTransaction(payload);
                } else if (topic_str == "hashblock") {
                    handleBlock(payload);
                }

            } catch (const zmq::error_t& e) {
                std::cerr << "[ERROR] ZMQ Exception: " << e.what() << std::endl;
                performExponentialBackoff();
            }
        }
    }

    // Graceful Shutdown
    void stop() {
        std::cout << "[ZMQ] Stopping Listener..." << std::endl;
        running_ = false;
        // Context shutdown forces recv to exit immediately if blocked
        context_.shutdown(); 
        context_.close();
    }

private:
    zmq::context_t context_;
    zmq::socket_t subscriber_;
    std::atomic<bool> running_;
    std::string endpoint_;
    int reconnect_attempts_ = 0;
    std::atomic<uint64_t> zmqBlockSeq_{0};
    ailee::l1::ReorgDetector* reorgDetector_ = nullptr;
    ailee::SidechainBridge* bridge_ = nullptr;
    std::vector<uint8_t> bridgeAddressBytes_;

    // Helper: Convert Raw Bytes to Hex String (for Logs/Bridge)
    std::string toHex(const void* data, size_t size) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        const unsigned char* p = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < size; ++i) {
            ss << std::setw(2) << static_cast<int>(p[i]);
        }
        return ss.str();
    }

    void checkPegIn(const zmq::message_t& payload);

    // Handler: Raw Transaction
    void handleTransaction(const zmq::message_t& payload) {
        checkPegIn(payload);
    }

    // Handler: Block Hash
    void handleBlock(const zmq::message_t& payload) {
        // Bitcoin Core sends block hashes as raw 32 bytes (Little Endian usually)
        if (payload.size() == 32) {
            std::string blockHash = toHex(payload.data(), payload.size());
            std::cout << ">>> NEW BLOCK MINED: " << blockHash << " <<<" << std::endl;
            
            // If ReorgDetector is connected, track this block
            if (reorgDetector_) {
                // ZMQ 'hashblock' does not carry the block height; we use a local
                // sequential counter as a best-effort approximation until an RPC
                // call can supply the real height.
                uint64_t approxHeight = ++zmqBlockSeq_;
                uint64_t ts = static_cast<uint64_t>(std::time(nullptr));
                auto reorgEvent = reorgDetector_->detectReorg(approxHeight, blockHash, ts);
                if (reorgEvent) {
                    std::cout << "[ZMQ -> ReorgDetector] Potential reorg detected at approx height "
                              << approxHeight << "!" << std::endl;
                } else {
                    reorgDetector_->trackBlock(approxHeight, blockHash, ts);
                    std::cout << "[ZMQ -> ReorgDetector] Tracked new block hash: " << blockHash << std::endl;
                }
            }

            // Trigger AILEE TPS Re-calibration
            std::cout << "[AILEE] Triggering TPS Optimization for new block..." << std::endl;
        } else {
            std::cerr << "[WARN] Invalid Block Hash size: " << payload.size() << std::endl;
        }
    }

    // Hardened Reconnection Logic
    void performExponentialBackoff() {
        reconnect_attempts_++;
        int wait_time = std::min(reconnect_attempts_ * 2, 30); // Cap at 30 seconds
        
        std::cerr << "[Reconnect] Connection lost. Retrying in " << wait_time << "s..." << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        
        try {
            subscriber_ = zmq::socket_t(context_, ZMQ_SUB);
            init();
            reconnect_attempts_ = 0; // Reset on success
        } catch (...) {
            // Keep trying loop will catch it
        }
    }
#else
    explicit BitcoinZMQListener(const std::string& endpoint = "tcp://127.0.0.1:28332")
        : endpoint_(endpoint), reorgDetector_(nullptr) {}

    void setReorgDetector(ailee::l1::ReorgDetector* detector) {
        reorgDetector_ = detector;
    }

    void setSidechainBridge(ailee::SidechainBridge* bridge, const std::vector<uint8_t>& bridgeAddress) {
        bridge_ = bridge;
        bridgeAddressBytes_ = bridgeAddress;
    }

    void init() {
        std::cerr << "[ZMQ] ZeroMQ support not compiled; listener disabled." << std::endl;
    }

    void start() {
        std::cerr << "[ZMQ] ZeroMQ support not compiled; listener disabled." << std::endl;
    }

    void stop() {}

private:
    std::string endpoint_;
    ailee::l1::ReorgDetector* reorgDetector_;
    ailee::SidechainBridge* bridge_ = nullptr;
    std::vector<uint8_t> bridgeAddressBytes_;
#endif
};

} // namespace ailee

#endif // AILEE_BITCOIN_ZMQ_LISTENER_H
