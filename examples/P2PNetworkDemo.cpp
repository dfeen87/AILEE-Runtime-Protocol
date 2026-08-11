// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
// P2PNetworkDemo.cpp - Demonstration of P2P networking with libp2p
// 
// This example shows how to:
// 1. Initialize a P2P network node
// 2. Connect to peers
// 3. Subscribe to topics
// 4. Publish and receive messages
// 5. Monitor network statistics

#include "network/P2PNetwork.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <csignal>

using namespace ailee::network;

// Global network pointer for signal handler
static P2PNetwork* g_network = nullptr;

// Global flag for graceful shutdown
static volatile bool running = true;

void signalHandler(int signal) {
    std::cout << "\n[Demo] Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Helper to convert bytes to hex string
std::string bytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

// Message handler for task distribution topic
void onTaskMessage(const NetworkMessage& msg) {
    std::cout << "\n[Task Handler] Received message:" << std::endl;
    std::cout << "  From: " << msg.senderId << std::endl;
    std::cout << "  Topic: " << msg.topic << std::endl;
    std::cout << "  Timestamp: " << msg.timestamp << std::endl;
    std::cout << "  Message ID: " << msg.messageId << std::endl;
    std::cout << "  Payload (hex): " << bytesToHex(msg.payload) << std::endl;
    std::cout << "  Payload (size): " << msg.payload.size() << " bytes" << std::endl;
}

// Message handler for telemetry topic
void onTelemetryMessage(const NetworkMessage& msg) {
    std::cout << "\n[Telemetry Handler] Received telemetry data:" << std::endl;
    std::cout << "  From: " << msg.senderId << std::endl;
    std::cout << "  Size: " << msg.payload.size() << " bytes" << std::endl;
}

// Message handler for heartbeat topic
void onHeartbeatMessage(const NetworkMessage& msg) {
    std::cout << "[Heartbeat] From: " << msg.senderId.substr(0, 16) << "..." << std::endl;
}

void printNetworkStats(const P2PNetwork& network) {
    auto stats = network.getStats();
    
    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              Network Statistics                        ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  Connected Peers:      " << std::setw(5) << stats.connectedPeers << "                         ║" << std::endl;
    std::cout << "║  Messages Sent:        " << std::setw(5) << stats.totalMessagesSent << "                         ║" << std::endl;
    std::cout << "║  Messages Received:    " << std::setw(5) << stats.totalMessagesReceived << "                         ║" << std::endl;
    std::cout << "║  Bytes Uploaded:       " << std::setw(10) << stats.bytesUploaded << " bytes            ║" << std::endl;
    std::cout << "║  Bytes Downloaded:     " << std::setw(10) << stats.bytesDownloaded << " bytes            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
}

void printPeerList(const P2PNetwork& network) {
    auto peers = network.getPeers();
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                         Connected Peers                                 ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════════════════╣" << std::endl;
    
    if (peers.empty()) {
        std::cout << "║  No peers connected                                                    ║" << std::endl;
    } else {
        for (const auto& peer : peers) {
            std::string peerId = peer.peerId.substr(0, 20) + "...";
            std::cout << "║  " << std::left << std::setw(70) << peerId << "║" << std::endl;
            std::cout << "║    Address: " << std::left << std::setw(59) << peer.multiaddr << "║" << std::endl;
            std::cout << "║    Latency: " << std::right << std::setw(4) << peer.latencyMs << "ms" 
                      << std::left << std::setw(56) << "" << "║" << std::endl;
            std::cout << "║    Status: " << std::left << std::setw(60) 
                      << (peer.connected ? "Connected" : "Disconnected") << "║" << std::endl;
            std::cout << "╟────────────────────────────────────────────────────────────────────────╢" << std::endl;
        }
    }
    
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << "   AILEE-Core P2P Network Demonstration" << std::endl;
    std::cout << "   libp2p C++ Integration Example" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    
    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Parse command line arguments
    std::string listenAddr = "/ip4/0.0.0.0/tcp/4001";
    std::vector<std::string> bootstrapPeers;
    
    if (argc > 1) {
        listenAddr = argv[1];
    }
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            bootstrapPeers.+= argv[i]);
        }
    }
    
    // Configure P2P network
    P2PConfig config;
    config.listenAddress = listenAddr;
    config.bootstrapPeers = bootstrapPeers;
    config.privateKeyPath = "./data/demo_p2p_key";
    config.maxPeers = 50;
    config.enableMDNS = true;
    config.enableDHT = true;
    
    std::cout << "[Demo] Configuration:" << std::endl;
    std::cout << "  Listen Address: " << config.listenAddress << std::endl;
    std::cout << "  Max Peers: " << config.maxPeers << std::endl;
    std::cout << "  mDNS: " << (config.enableMDNS ? "enabled" : "disabled") << std::endl;
    std::cout << "  DHT: " << (config.enableDHT ? "enabled" : "disabled") << std::endl;
    std::cout << "  Bootstrap Peers: " << config.bootstrapPeers.size() << std::endl;
    for (const auto& peer : config.bootstrapPeers) {
        std::cout << "    - " << peer << std::endl;
    }
    std::cout << std::endl;
    
    // Create and start P2P network
    std::cout << "[Demo] Creating P2P network..." << std::endl;
    P2PNetwork network(config);
    
    std::cout << "[Demo] Starting network..." << std::endl;
    if (!network.start()) {
        std::cerr << "[Demo] Failed to start P2P network" << std::endl;
        return 1;
    }
    
    std::cout << "[Demo] Network started successfully!" << std::endl;
    std::cout << "[Demo] Local Peer ID: " << network.getLocalPeerId() << std::endl;
    std::cout << std::endl;
    
    // Subscribe to topics
    std::cout << "[Demo] Subscribing to topics..." << std::endl;
    network.subscribe("ailee.tasks", onTaskMessage);
    network.subscribe("ailee.telemetry", onTelemetryMessage);
    network.subscribe("ailee.heartbeat", onHeartbeatMessage);
    std::cout << "[Demo] Subscribed to 3 topics" << std::endl;
    std::cout << std::endl;
    
    // Publish initial messages
    std::cout << "[Demo] Publishing initial messages..." << std::endl;
    
    // Publish task message
    std::vector<uint8_t> taskPayload = {0x01, 0x02, 0x03, 0x04, 0x05};
    network.publish("ailee.tasks", taskPayload);
    
    // Publish telemetry message
    std::vector<uint8_t> telemetryPayload = {0x10, 0x20, 0x30, 0x40};
    network.publish("ailee.telemetry", telemetryPayload);
    
    std::cout << "[Demo] Initial messages published" << std::endl;
    std::cout << std::endl;
    
    // Main demo loop
    std::cout << "[Demo] Entering main loop (Ctrl+C to exit)..." << std::endl;
    std::cout << std::endl;
    
    int iteration = 0;
    while (running) {
        iteration++;
        
        // Publish heartbeat every 5 iterations
        if (iteration % 5 == 0) {
            std::vector<uint8_t> heartbeat = {0xFF, 0xFE, 0xFD};
            network.publish("ailee.heartbeat", heartbeat);
        }
        
        // Print statistics every 10 iterations
        if (iteration % 10 == 0) {
            printNetworkStats(network);
            printPeerList(network);
        }
        
        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Cleanup
    std::cout << "\n[Demo] Shutting down..." << std::endl;
    
    // Unsubscribe from topics
    network.unsubscribe("ailee.tasks");
    network.unsubscribe("ailee.telemetry");
    network.unsubscribe("ailee.heartbeat");
    
    // Print final statistics
    printNetworkStats(network);
    
    // Stop network
    network.stop();
    
    std::cout << "\n[Demo] Demo completed successfully" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    
    return 0;
}
