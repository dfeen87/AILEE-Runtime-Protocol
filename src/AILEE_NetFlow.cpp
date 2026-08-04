// Licensed under the PolyForm Noncommercial License 1.0.0
// AILEE_NetFlow.cpp — Implementation for Hybrid Layer-2 Decentralized Internet
// Integrates AmbientAI telemetry, ZK proof-of-bandwidth, token rewards, and relay scheduling

#include "ailee_netflow.h"
#include <chrono>
#include <thread>
#include <random>
#include <iostream>
#include <algorithm>

namespace ailee_netflow {

// ----------------- RelayNode Utilities -----------------

void markNodeOnline(RelayNode& node) {
    node.online = true;
    node.lastSeen = std::chrono::system_clock::now();
}

void markNodeOffline(RelayNode& node) {
    node.online = false;
}

// ----------------- NetFlowTunnel Implementation -----------------

double NetFlowTunnel::relayBandwidth(double requestedMbps) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!active_ || node_.advertisedBandwidthMbps <= 0) return 0.0;

    double allocated = std::min(requestedMbps, node_.advertisedBandwidthMbps);

    // Optional: Apply hybrid routing penalty for Onion hops
    if (mode_ == TunnelMode::Onion) {
        allocated *= 0.85; // ~15% overhead
    } else if (mode_ == TunnelMode::Hybrid) {
        allocated *= 0.95; // small overhead
    }

    node_.advertisedBandwidthMbps = std::max(0.0, node_.advertisedBandwidthMbps - allocated);
    node_.lastSeen = std::chrono::system_clock::now();
    return allocated;
}

// ----------------- NetFlowMesh Implementation -----------------

std::optional<RelayNode> NetFlowMesh::selectNode(double minBandwidthMbps) {
    std::lock_guard<std::mutex> lock(mu_);
    RelayNode* best = nullptr;
    double bestScore = -1.0;

    for (auto& [_, n] : nodes_) {
        if (!n.online) continue;
        if (n.advertisedBandwidthMbps < minBandwidthMbps) continue;

        // Score can include latency, uptime, ZK proof validation
        double score = n.advertisedBandwidthMbps;
        if (score > bestScore) { bestScore = score; best = &n; }
    }

    if (best) return *best;
    return std::nullopt;
}

TokenizedBandwidth NetFlowMesh::rewardNode(const RelayNode& node, double bandwidthUsed, double baseRate) {
    TokenizedBandwidth t;
    t.nodePubkey = node.id.pubkey;
    t.tokensEarned = bandwidthUsed * baseRate;
    t.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

    // Generate ZK proof for bandwidth allocation
    ailee::zk::ZKEngine zkEngine;
    auto proof = zkEngine.generateProof(node.id.pubkey, std::to_string(t.tokensEarned));
    t.zkProofHash = proof.proofData;
    return t;
}

// ----------------- HybridNetFlow Implementation -----------------

double HybridNetFlow::pushTraffic(double requestedMbps) {
    std::lock_guard<std::mutex> lock(mu_);
    double remaining = requestedMbps;

    // Allocate traffic across all active tunnels
    for (auto& t : tunnels_) {
        if (!t.isActive()) continue;
        double allocated = t.relayBandwidth(remaining);
        remaining -= allocated;
        if (remaining <= 0) break;
    }

    return requestedMbps - remaining; // actual delivered
}

std::vector<NetFlowTunnel> HybridNetFlow::activeTunnels() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<NetFlowTunnel> active;
    for (auto& t : tunnels_) if (t.isActive()) active.push_back(t);
    return active;
}

// ----------------- Auto Scheduler / Simulator -----------------

void simulateNetworkLoad(HybridNetFlow& net, double totalMbps, double intervalMs = 1000.0) {
    std::default_random_engine rng(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    );
    std::uniform_real_distribution<double> dist(0.5, 1.5);

    double remaining = totalMbps;
    while (remaining > 0) {
        double requested = dist(rng) * 10; // random small chunk
        double delivered = net.pushTraffic(requested);
        std::cout << "[NetFlow] Requested: " << requested
                  << " Mbps, Delivered: " << delivered << " Mbps\n";
        remaining -= delivered;
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(intervalMs)));
    }
}

// ----------------- Node Bandwidth Refill -----------------

void refillNodeBandwidth(NetFlowMesh& mesh, double refillMbps) {
    mesh.refillAll(refillMbps);
}

} // namespace ailee_netflow
