// SPDX-License-Identifier: MIT
// P2PNetwork.cpp — P2P networking implementation with libp2p C++ bindings

#include "P2PNetwork.h"
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

// Conditional compilation: Use libp2p if available, otherwise use enhanced stub
#ifdef AILEE_HAS_LIBP2P
// Include libp2p C++ headers when available
// #include <libp2p/host/host.hpp>
// #include <libp2p/peer/peer_id.hpp>
// #include <libp2p/protocol/gossip/gossipsub.hpp>
// #include <libp2p/protocol/kademlia/kademlia.hpp>
// #include <libp2p/transport/tcp.hpp>
// Note: Actual includes will be enabled when libp2p is installed
#define USING_LIBP2P 0
#else
#define USING_LIBP2P 0
#endif

namespace ailee::network {

// ============================================================================
// P2PNetwork::Impl - Private Implementation
// ============================================================================

class P2PNetwork::Impl {
public:
    P2PConfig config;
    bool running = false;
    std::string localPeerId;
    std::vector<PeerInfo> peers;
    std::map<std::string, MessageHandler> subscriptions;
    std::mutex mutex;
    NetworkStats stats{};
    ReputationRateLimiter rateLimiter;
    
    // Background thread for peer discovery and message handling
    std::thread backgroundThread;
    
#if USING_LIBP2P
    // libp2p components
    std::shared_ptr<libp2p::Host> host;
    std::shared_ptr<libp2p::protocol::Gossipsub> gossipsub;
    std::shared_ptr<libp2p::protocol::Kademlia> kademlia;
#endif
    
    Impl(const P2PConfig& cfg) : config(cfg) {
        localPeerId = loadOrGeneratePeerId();
    }
    
    ~Impl() {
        // Ensure clean shutdown
        running = false;
        if (backgroundThread.joinable()) {
            backgroundThread.join();
        }
    }
    
    bool initialize() {
#if USING_LIBP2P
        std::cout << "[P2PNetwork] Initializing with libp2p C++ bindings" << std::endl;

        // Note: Real initialization requires a fully configured libp2p Injector.
        // For migration prep, we simulate the architecture layout.

        // host = libp2p::Host::create(config.listenAddress);
        // if (!host) return false;
        
        // Initialize Kademlia DHT if enabled
        // if (config.enableDHT) {
        //     kademlia = std::make_shared<libp2p::protocol::Kademlia>(host);
        //     kademlia->start();
        // }
        
        // Initialize GossipSub for pub/sub (state diffs, zk proofs, telemetry)
        // gossipsub = std::make_shared<libp2p::protocol::Gossipsub>(host);
        // gossipsub->start();
        
        // Connect to bootstrap peers
        // for (const auto& peer : config.bootstrapPeers) {
        //     host->connect(peer);
        // }
        
        return true;
#else
        std::cout << "[P2PNetwork] Running in enhanced stub mode (libp2p not available)" << std::endl;

        
        // Start background simulation thread
        backgroundThread = std::thread([this]() {
            simulateNetworkActivity();
        });
        
        return true;
#endif
    }
    
    void cleanup() {
#if USING_LIBP2P
        // Stop libp2p components
        if (gossipsub) {
            // gossipsub->stop();
        }
        if (kademlia) {
            // kademlia->stop();
        }
        if (host) {
            // host->close();
        }
#else
        // Stub cleanup - just mark as stopped
        std::cout << "[P2PNetwork] Cleaning up stub resources" << std::endl;
#endif
    }
    
    bool subscribeToTopic(const std::string& topic, MessageHandler handler) {
#if USING_LIBP2P
        // Subscribe via libp2p GossipSub

        // std::cout << "[P2PNetwork] Subscribing to topic via libp2p: " << topic << std::endl;
        // return gossipsub->subscribe(topic, [handler, topic](const auto& msg) {
        //     NetworkMessage netMsg;
        //     netMsg.topic = topic;
        //     netMsg.payload = msg.data;
        //     netMsg.senderId = msg.from.toBase58();
        //     netMsg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        //     handler(netMsg);
        // });
        return true;
#else
        subscriptions[topic] = handler;
        std::cout << "[P2PNetwork] Subscribed to topic: " << topic << " (stub mode)" << std::endl;
        return true;
#endif
    }
    
    bool publishToTopic(const std::string& topic, const std::vector<uint8_t>& payload) {
#if USING_LIBP2P
        // Publish via libp2p GossipSub
        // std::cout << "[P2PNetwork] Publishing to topic via libp2p: " << topic << std::endl;
        // return gossipsub->publish(topic, payload);
        return true;
#else
        std::cout << "[P2PNetwork] Publishing to topic: " << topic 
                  << " (size: " << payload.size() << " bytes, stub mode)" << std::endl;
        
        // Simulate local delivery if subscribed
        auto it = subscriptions.find(topic);
        if (it != subscriptions.end()) {
            NetworkMessage msg;
            msg.senderId = localPeerId;
            msg.topic = topic;
            msg.payload = payload;
            msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            msg.messageId = generateMessageId();
            
            // Deliver synchronously to avoid detached thread issues on shutdown
            try {
                it->second(msg);
            } catch (const std::exception& e) {
                std::cerr << "[P2PNetwork] Error in message handler: " << e.what() << std::endl;
            }
        }
        
        stats.totalMessagesSent++;
        stats.bytesUploaded += payload.size();
        return true;
#endif
    }
    
    bool connectPeer(const std::string& multiaddr) {
#if USING_LIBP2P
        // Connect via libp2p
        // auto result = host->connect(multiaddr);
        // return result.has_value();
        return true;
#else
        std::cout << "[P2PNetwork] Connecting to peer: " << multiaddr << " (stub mode)" << std::endl;
        
        // Simulate peer connection
        PeerInfo peer;
        peer.peerId = generatePeerId();
        peer.multiaddr = multiaddr;
        peer.connected = true;
        peer.lastSeen = std::chrono::system_clock::now().time_since_epoch().count();
        peer.latencyMs = 50; // Simulated latency
        
        std::lock_guard<std::mutex> lock(mutex);
        peers.push_back(peer);
        
        return true;
#endif
    }
    
private:
    std::string loadOrGeneratePeerId() {
        // Try to load existing peer ID from config file
        if (!config.privateKeyPath.empty()) {
            std::ifstream keyFile(config.privateKeyPath);
            if (keyFile.is_open()) {
                std::string peerId;
                std::getline(keyFile, peerId);
                if (!peerId.empty()) {
                    std::cout << "[P2PNetwork] Loaded peer ID from: " << config.privateKeyPath << std::endl;
                    return peerId;
                }
            }
        }
        
        // Generate new peer ID
        auto peerId = generatePeerId();
        
        // Save to file if path provided
        if (!config.privateKeyPath.empty()) {
            std::ofstream keyFile(config.privateKeyPath);
            if (keyFile.is_open()) {
                keyFile << peerId;
                std::cout << "[P2PNetwork] Saved new peer ID to: " << config.privateKeyPath << std::endl;
            }
        }
        
        return peerId;
    }
    
    static std::string generatePeerId() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        // Generate libp2p-compatible peer ID (base58 encoded)
        std::stringstream ss;
        ss << "Qm"; // libp2p peer ID prefix
        for (int i = 0; i < 44; i++) { // Standard length
            static const char charset[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
            ss << charset[dis(gen) % (sizeof(charset) - 1)];
        }
        return ss.str();
    }
    
    static std::string generateMessageId() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        for (int i = 0; i < 32; i++) {
            ss << std::hex << dis(gen);
        }
        return ss.str();
    }
    
    // Background thread for simulating network activity (stub mode only)
    void simulateNetworkActivity() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            // Simulate peer discovery
            if (peers.size() < config.maxPeers / 2) {
                PeerInfo peer;
                peer.peerId = generatePeerId();
                peer.multiaddr = "/ip4/192.168.1." + std::to_string(100 + peers.size()) + "/tcp/4001";
                peer.connected = true;
                peer.lastSeen = std::chrono::system_clock::now().time_since_epoch().count();
                peer.latencyMs = 20 + (peers.size() * 5);
                
                std::lock_guard<std::mutex> lock(mutex);
                peers.push_back(peer);
                
                std::cout << "[P2PNetwork] Discovered peer (simulated): " << peer.peerId << std::endl;
            }
        }
    }
};

// ============================================================================
// P2PNetwork - Public Interface Implementation
// ============================================================================

P2PNetwork::P2PNetwork(const P2PConfig& config)
    : impl_(std::make_unique<Impl>(config)) {
}

P2PNetwork::~P2PNetwork() = default;

bool P2PNetwork::start() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->running) return true;
    std::cout << "[P2PNetwork] Starting P2P network layer" << std::endl;
    int res = init_network_ffi();
    if (res != 0) {
        std::cerr << "[P2PNetwork] Failed to initialize rust-libp2p network via FFI" << std::endl;
        return false;
    }
    impl_->running = true;
    std::cout << "[P2PNetwork] Network started successfully" << std::endl;
    return true;
}

void P2PNetwork::stop() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        return;
    }
    
    std::cout << "[P2PNetwork] Stopping P2P network" << std::endl;
    impl_->running = false;
    impl_->cleanup();
    
    std::cout << "[P2PNetwork] Network stopped" << std::endl;
}

bool P2PNetwork::isRunning() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->running;
}

std::string P2PNetwork::getLocalPeerId() const {
    return impl_->localPeerId;
}

std::vector<PeerInfo> P2PNetwork::getPeers() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->peers;
}

bool P2PNetwork::subscribe(const std::string& topic, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        std::cerr << "[P2PNetwork] Cannot subscribe: network not running" << std::endl;
        return false;
    }
    
    int res = subscribe_topic_ffi(topic.c_str());
    if (res == 0) {
        impl_->subscriptions[topic] = handler;
        return true;
    }
    return false;
}

bool P2PNetwork::unsubscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    impl_->subscriptions.erase(topic);
    std::cout << "[P2PNetwork] Unsubscribed from topic: " << topic << std::endl;
    
#if USING_LIBP2P
    // Unsubscribe via libp2p pubsub
    // return impl_->gossipsub->unsubscribe(topic);
#endif
    
    return true;
}

void P2PNetwork::handleIncomingMessage(const NetworkMessage& msg, double peerReputation) {
    MessageHandler handler;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->running) return;

        if (!impl_->rateLimiter.allowMessage(msg.senderId, peerReputation, msg.topic, msg.payload)) {
            std::cout << "[P2PNetwork] Rate limiter dropped message from peer " << msg.senderId
                      << " on topic " << msg.topic << std::endl;
            return;
        }

        auto it = impl_->subscriptions.find(msg.topic);
        if (it != impl_->subscriptions.end()) {
            handler = it->second;
        }
    }

    if (handler) {
        try {
            handler(msg);
        } catch (const std::exception& e) {
            std::cerr << "[P2PNetwork] Error in message handler: " << e.what() << std::endl;
        }
    }
}

bool P2PNetwork::publish(const std::string& topic, const std::vector<uint8_t>& payload) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        std::cerr << "[P2PNetwork] Cannot publish: network not running" << std::endl;
        return false;
    }

    // Since we are publishing locally to the network, we don't strict limit ourselves.
    // In a full node, incoming messages from peers would hit allowMessage() before
    // being processed or re-gossiped.
    
    int res = broadcast_message_ffi(topic.c_str(), payload.data(), payload.size());
    return res == 0;
}

// Add an overloaded publish with rate limit check for incoming messages from peers,
// or simulate incoming message checking in your libp2p incoming handler when it's fully implemented.
// Currently P2PNetwork mainly broadcasts out. To limit INCOMING, there should be a receive handler here.

std::optional<std::vector<uint8_t>> P2PNetwork::sendToPeer(
    const std::string& peerId,
    const std::string& protocol,
    const std::vector<uint8_t>& payload) {
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        std::cerr << "[P2PNetwork] Cannot send: network not running" << std::endl;
        return std::nullopt;
    }
    
    std::cout << "[P2PNetwork] Sending to peer: " << peerId 
              << " (protocol: " << protocol << ", size: " << payload.size() << " bytes)" << std::endl;
    
    impl_->stats.totalMessagesSent++;
    impl_->stats.bytesUploaded += payload.size();
    
#if USING_LIBP2P
    // Send via libp2p stream
    // auto stream = impl_->host->newStream(peerId, protocol);
    // if (!stream) return std::nullopt;
    // 
    // stream->write(payload);
    // auto response = stream->read();
    // 
    // impl_->stats.totalMessagesReceived++;
    // impl_->stats.bytesDownloaded += response.size();
    // 
    // return response;
#endif
    
    return std::nullopt;
}

bool P2PNetwork::connectToPeer(const std::string& multiaddr) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        std::cerr << "[P2PNetwork] Cannot connect: network not running" << std::endl;
        return false;
    }
    
    return impl_->connectPeer(multiaddr);
}

bool P2PNetwork::disconnectPeer(const std::string& peerId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    std::cout << "[P2PNetwork] Disconnecting peer: " << peerId << std::endl;
    
    // Remove from peers list
    impl_->peers.erase(
        std::remove_if(impl_->peers.begin(), impl_->peers.end(),
            [&peerId](const PeerInfo& p) { return p.peerId == peerId; }),
        impl_->peers.end()
    );
    
#if USING_LIBP2P
    // Disconnect via libp2p
    // impl_->host->disconnect(peerId);
#endif
    
    return true;
}

P2PNetwork::NetworkStats P2PNetwork::getStats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto stats = impl_->stats;
    stats.connectedPeers = impl_->peers.size();
    return stats;
}

} // namespace ailee::network
