// SPDX-License-Identifier: MIT
// P2PNetwork.h — Peer-to-peer networking layer for distributed AILEE nodes

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>

namespace ailee::network {

// Standard Gossipsub Topics for Validator Workflows
constexpr const char* TOPIC_L2_STATE_DIFF = "ailee/l2/state_diff";
constexpr const char* TOPIC_L2_ZK_PROOF = "ailee/l2/zk_proof";
constexpr const char* TOPIC_AI_TELEMETRY = "ailee/ai/telemetry";
constexpr const char* TOPIC_PROOF_FRAGMENT = "ailee/l2/proof_fragment";
constexpr const char* TOPIC_DISPUTE_EVENT = "ailee/l2/dispute_event";

/**
 * Peer information structure
 */
struct PeerInfo {
    std::string peerId;           // Unique peer identifier
    std::string multiaddr;        // Multi-address (e.g., /ip4/127.0.0.1/tcp/4001)
    std::string publicKey;        // Peer's public key
    uint64_t lastSeen;            // Timestamp of last communication
    uint32_t latencyMs;           // Measured latency
    bool connected;               // Connection status
};

/**
 * Network message structure
 */
struct NetworkMessage {
    std::string senderId;         // Sender peer ID
    std::string topic;            // Message topic/channel
    std::vector<uint8_t> payload; // Message payload
    uint64_t timestamp;           // Message timestamp
    std::string messageId;        // Unique message identifier
};

/**
 * P2P Network configuration
 */
struct P2PConfig {
    std::string listenAddress;
    std::vector<std::string> bootstrapPeers;
    std::string privateKeyPath;
    uint32_t maxPeers;
    bool enableMDNS;        // Local peer discovery
    bool enableDHT;         // Distributed hash table
    
    P2PConfig()
        : listenAddress("/ip4/0.0.0.0/tcp/4001")
        , privateKeyPath("./data/p2p_private_key")
        , maxPeers(50)
        , enableMDNS(true)
        , enableDHT(true)
    {}
};

/**
 * Message handler callback type
 */
using MessageHandler = std::function<void(const NetworkMessage&)>;

/**
 * P2P Network layer using libp2p
 * 
 * Provides:
 * - Peer discovery (mDNS, DHT, bootstrap)
 * - Pub/Sub messaging for task distribution
 * - Direct peer-to-peer communication
 * - Connection management
 */
class P2PNetwork {
public:
    explicit P2PNetwork(const P2PConfig& config = P2PConfig{});
    ~P2PNetwork();
    
    // Disable copy, allow move
    P2PNetwork(const P2PNetwork&) = delete;
    P2PNetwork& operator=(const P2PNetwork&) = delete;
    P2PNetwork(P2PNetwork&&) = default;
    P2PNetwork& operator=(P2PNetwork&&) = default;
    
    /**
     * Start the P2P network
     * @return true if successful
     */
    bool start();
    
    /**
     * Stop the P2P network
     */
    void stop();
    
    /**
     * Check if network is running
     */
    bool isRunning() const;
    
    /**
     * Get local peer ID
     */
    std::string getLocalPeerId() const;
    
    /**
     * Get list of connected peers
     */
    std::vector<PeerInfo> getPeers() const;
    
    /**
     * Subscribe to a topic for pub/sub messaging
     * @param topic Topic name
     * @param handler Message handler callback
     * @return true if successful
     */
    bool subscribe(const std::string& topic, MessageHandler handler);
    
    /**
     * Handle incoming message from network, checking rate limits
     */
    void handleIncomingMessage(const NetworkMessage& msg, double peerReputation = 0.5);

    /**
     * Unsubscribe from a topic
     */
    bool unsubscribe(const std::string& topic);
    
    /**
     * Publish message to a topic
     * @param topic Topic name
     * @param payload Message payload
     * @return true if successful
     */
    bool publish(const std::string& topic, const std::vector<uint8_t>& payload);
    
    /**
     * Send direct message to a peer
     * @param peerId Target peer ID
     * @param protocol Protocol identifier
     * @param payload Message payload
     * @return Response payload (if any)
     */
    std::optional<std::vector<uint8_t>> sendToPeer(
        const std::string& peerId,
        const std::string& protocol,
        const std::vector<uint8_t>& payload
    );
    
    /**
     * Connect to a peer by multiaddress
     */
    bool connectToPeer(const std::string& multiaddr);
    
    /**
     * Disconnect from a peer
     */
    bool disconnectPeer(const std::string& peerId);
    
    /**
     * Get network statistics
     */
    struct NetworkStats {
        size_t connectedPeers;
        size_t totalMessagesSent;
        size_t totalMessagesReceived;
        uint64_t bytesUploaded;
        uint64_t bytesDownloaded;
    };
    NetworkStats getStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ailee::network
