// SPDX-License-Identifier: MIT
// P2PNetwork.cpp — P2P networking implementation with libp2p C++ bindings

#include "P2PNetwork.h"
#include "LogicalClock.h"
#include "ailee_rust_ffi.h"
#include "ReputationRateLimiter.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>

namespace ailee::network {

// ============================================================================
// NetworkMessage Serialization
// ============================================================================

namespace {
    void writeString(std::vector<uint8_t>& buf, const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.size());
        buf.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
        buf.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
        buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>(len & 0xFF));
        buf.insert(buf.end(), str.begin(), str.end());
    }

    bool readString(const uint8_t*& data, size_t& len, std::string& str) {
        if (len < 4) return false;
        uint32_t strLen = (static_cast<uint32_t>(data[0]) << 24) |
                          (static_cast<uint32_t>(data[1]) << 16) |
                          (static_cast<uint32_t>(data[2]) << 8) |
                           static_cast<uint32_t>(data[3]);
        data += 4;
        len -= 4;

        if (len < strLen) return false;
        str.assign(reinterpret_cast<const char*>(data), strLen);
        data += strLen;
        len -= strLen;
        return true;
    }

    void writeUint64(std::vector<uint8_t>& buf, uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            buf.push_back((val >> (i * 8)) & 0xFF);
        }
    }

    bool readUint64(const uint8_t*& data, size_t& len, uint64_t& val) {
        if (len < 8) return false;
        val = 0;
        for (int i = 0; i < 8; ++i) {
            val = (val << 8) | data[i];
        }
        data += 8;
        len -= 8;
        return true;
    }

    void writeBytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& bytes) {
        uint32_t len = static_cast<uint32_t>(bytes.size());
        buf.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
        buf.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
        buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>(len & 0xFF));
        buf.insert(buf.end(), bytes.begin(), bytes.end());
    }

    bool readBytes(const uint8_t*& data, size_t& len, std::vector<uint8_t>& bytes) {
        if (len < 4) return false;
        uint32_t bytesLen = (static_cast<uint32_t>(data[0]) << 24) |
                            (static_cast<uint32_t>(data[1]) << 16) |
                            (static_cast<uint32_t>(data[2]) << 8) |
                             static_cast<uint32_t>(data[3]);
        data += 4;
        len -= 4;

        if (len < bytesLen) return false;
        bytes.assign(data, data + bytesLen);
        data += bytesLen;
        len -= bytesLen;
        return true;
    }
}

std::vector<uint8_t> NetworkMessage::serialize() const {
    std::vector<uint8_t> buf;
    writeString(buf, senderId);
    writeString(buf, topic);
    writeBytes(buf, payload);
    writeUint64(buf, timestamp);
    writeString(buf, messageId);
    return buf;
}

bool NetworkMessage::deserialize(const uint8_t* data, size_t len) {
    if (!readString(data, len, senderId)) return false;
    if (!readString(data, len, topic)) return false;
    if (!readBytes(data, len, payload)) return false;
    if (!readUint64(data, len, timestamp)) return false;
    if (!readString(data, len, messageId)) return false;
    return true; // We don't care if there's extra data at the end for future compatibility
}

// ============================================================================
// FFINetworkTransport - Concrete Implementation wrapping Rust libp2p FFI
// ============================================================================

class FFINetworkTransport : public INetworkTransport {
public:
    FFINetworkTransport(const P2PConfig& cfg) : config(cfg) {
        localPeerId = "local_node"; // For real integration, fetch from FFI or config
    }

    ~FFINetworkTransport() override {
        stop();
    }

    bool start() override {
        std::lock_guard<std::mutex> lock(mutex);
        if (running) return true;

        std::cout << "[FFINetworkTransport] Starting FFI network layer" << std::endl;
        
        // Initialize Rust FFI network
        int res = init_network_ffi();
        if (res != 0) {
            std::cerr << "[FFINetworkTransport] Failed to initialize rust-libp2p network via FFI" << std::endl;
            return false;
        }

        running = true;
        std::cout << "[FFINetworkTransport] Network started successfully" << std::endl;
        return true;
    }
    
    void stop() override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return;

        running = false;
    }

    bool isRunning() const override {
        return running;
    }

    std::string getLocalPeerId() const override {
        return localPeerId;
    }

    std::vector<PeerInfo> getPeers() const override {
        std::lock_guard<std::mutex> lock(mutex);
        return std::vector<PeerInfo>(); // Placeholder; fetch from FFI in complete implementation
    }

    bool subscribe([[maybe_unused]] const std::string& topic, [[maybe_unused]] MessageHandler handler) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return false;

        if (subscribe_topic_ffi(topic.c_str()) != 0) {
            std::cerr << "[FFINetworkTransport] Failed to subscribe to topic: " << topic << std::endl;
            return false;
        }
        return true;
    }
    
    bool unsubscribe([[maybe_unused]] const std::string& topic) override {
        std::lock_guard<std::mutex> lock(mutex);
        return false; // Not implemented in current FFI
    }
    
    bool publish(const std::string& topic, [[maybe_unused]] const std::vector<uint8_t>& payload) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return false;

        int res = broadcast_message_ffi(topic.c_str(), payload.data(), payload.size());
        return res == 0;
    }

    std::optional<std::vector<uint8_t>> sendToPeer(
        [[maybe_unused]] const std::string& peerId,
        [[maybe_unused]] const std::string& protocol,
        [[maybe_unused]] const std::vector<uint8_t>& payload) override {
        
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return std::nullopt;

        return std::nullopt;
    }
    
    bool connectToPeer([[maybe_unused]] const std::string& multiaddr) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return false;

        return false; // Not implemented in current FFI
    }
    
    bool disconnectPeer([[maybe_unused]] const std::string& peerId) override {
        std::lock_guard<std::mutex> lock(mutex);
        return false; // Not implemented in current FFI
    }

private:
    P2PConfig config;
    std::string localPeerId;
    bool running = false;
    mutable std::mutex mutex;
};

// ============================================================================
// P2PNetwork - Public Interface Implementation
// ============================================================================

P2PNetwork::P2PNetwork(const P2PConfig& config)
    : transport_(std::make_unique<FFINetworkTransport>(config)) {
}

P2PNetwork::P2PNetwork(std::unique_ptr<INetworkTransport> transport)
    : transport_(std::move(transport)) {
}

P2PNetwork::~P2PNetwork() {
    if (transport_) {
        transport_->stop();
    }
}

bool P2PNetwork::start() {
    return transport_->start();
}

void P2PNetwork::stop() {
    transport_->stop();
}

bool P2PNetwork::isRunning() const {
    return transport_->isRunning();
}

std::string P2PNetwork::getLocalPeerId() const {
    return transport_->getLocalPeerId();
}

std::vector<PeerInfo> P2PNetwork::getPeers() const {
    return transport_->getPeers();
}

bool P2PNetwork::subscribe(const std::string& topic, MessageHandler handler) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_[topic] = handler;
    }
    
    // Create an interception handler to enforce rate limiting
    auto interceptor = [this, topic](const NetworkMessage& msg) {
        MessageHandler currentHandler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscriptions_.find(topic);
            if (it != subscriptions_.end()) {
                currentHandler = it->second;
            }
        }
        // We'll pass a default reputation for now; in a full node, reputation is fetched via other means
        this->internalMessageHandler(msg, currentHandler, 0.5);
    };

    return transport_->subscribe(topic, interceptor);
}

bool P2PNetwork::unsubscribe(const std::string& topic) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_.erase(topic);
    }
    return transport_->unsubscribe(topic);
}

void P2PNetwork::handleIncomingMessage(const NetworkMessage& msg, double peerReputation) {
    // Advance logical clock on received messages so rate limits actually work
    LogicalClock::next();

    MessageHandler handler;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscriptions_.find(msg.topic);
        if (it != subscriptions_.end()) {
            handler = it->second;
        }
    }
    if (handler) {
        internalMessageHandler(msg, handler, peerReputation);
    }
}

void P2PNetwork::internalMessageHandler(const NetworkMessage& msg, MessageHandler userHandler, double peerReputation) {
    if (!rateLimiter_.allowMessage(msg.senderId, peerReputation, msg.topic, msg.payload)) {
        std::cout << "[P2PNetwork] Rate limiter dropped message from peer " << msg.senderId
                  << " on topic " << msg.topic << std::endl;
        return;
    }

    if (userHandler) {
        try {
            userHandler(msg);
            std::lock_guard<std::mutex> lock(mutex_);
            internalStats_.totalMessagesReceived++;
            internalStats_.bytesDownloaded += msg.payload.size();
        } catch (const std::exception& e) {
            std::cerr << "[P2PNetwork] Error in message handler: " << e.what() << std::endl;
        }
    }
}

bool P2PNetwork::publish(const std::string& topic, const std::vector<uint8_t>& payload) {
    bool success = transport_->publish(topic, payload);
    if (success) {
        std::lock_guard<std::mutex> lock(mutex_);
        internalStats_.totalMessagesSent++;
        internalStats_.bytesUploaded += payload.size();
    }
    return success;
}

std::optional<std::vector<uint8_t>> P2PNetwork::sendToPeer(
    [[maybe_unused]] const std::string& peerId,
    [[maybe_unused]] const std::string& protocol,
    const std::vector<uint8_t>& payload) {
    
    auto result = transport_->sendToPeer(peerId, protocol, payload);
    if (result) {
        std::lock_guard<std::mutex> lock(mutex_);
        internalStats_.totalMessagesSent++;
        internalStats_.bytesUploaded += payload.size();
        internalStats_.totalMessagesReceived++;
        internalStats_.bytesDownloaded += result->size();
    }
    return result;
}

bool P2PNetwork::connectToPeer(const std::string& multiaddr) {
    return transport_->connectToPeer(multiaddr);
}

bool P2PNetwork::disconnectPeer(const std::string& peerId) {
    return transport_->disconnectPeer(peerId);
}

P2PNetwork::NetworkStats P2PNetwork::getStats() const {
    NetworkStats stats;
    stats.connectedPeers = transport_->getPeers().size();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats.totalMessagesSent = internalStats_.totalMessagesSent;
        stats.totalMessagesReceived = internalStats_.totalMessagesReceived;
        stats.bytesUploaded = internalStats_.bytesUploaded;
        stats.bytesDownloaded = internalStats_.bytesDownloaded;
    }
    return stats;
}

} // namespace ailee::network
