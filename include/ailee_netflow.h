// SPDX-License-Identifier: MIT
// ailee_netflow.h — Layer-2 Full-Relay Engine for Hybrid Decentralized Internet
// Integrates AmbientAI, ZK Proof-of-Bandwidth, tokenized incentives, and hybrid relay tunneling.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <atomic>
#include <mutex>
#include <functional>
#include <cstdint>

#include "AmbientAI.h"
#include "zk_proofs.h"

namespace ailee_netflow {

// ----------------- Node Networking -----------------

struct RelayNode {
    ambient::NodeId id;
    std::string publicIP;
    uint16_t port;
    bool online = false;
    double advertisedBandwidthMbps = 0.0;
    std::chrono::system_clock::time_point lastSeen;
};

enum class TunnelMode { WireGuard, Onion, Hybrid };

// ----------------- Bandwidth & Token Accounting -----------------

struct BandwidthReport {
    RelayNode node;
    double usedMbps = 0.0;
    double availableMbps = 0.0;
    std::chrono::system_clock::time_point timestamp;
};

struct TokenizedBandwidth {
    std::string nodePubkey;
    double tokensEarned = 0.0;
    uint64_t timestampMs = 0;
    std::string zkProofHash;
};

// ----------------- NetFlow Tunnel -----------------

class NetFlowTunnel {
public:
    NetFlowTunnel(RelayNode node, TunnelMode mode)
        : node_(std::move(node)), mode_(mode), active_(false) {}

    NetFlowTunnel(const NetFlowTunnel& other)
        : node_(other.node_), mode_(other.mode_), active_(other.active_.load()) {}

    NetFlowTunnel& operator=(const NetFlowTunnel& other) {
        if (this != &other) {
            std::lock_guard<std::mutex> lock(mu_);
            node_ = other.node_;
            mode_ = other.mode_;
            active_.store(other.active_.load());
        }
        return *this;
    }

    NetFlowTunnel(NetFlowTunnel&& other) noexcept
        : node_(std::move(other.node_)), mode_(other.mode_), active_(other.active_.load()) {}

    NetFlowTunnel& operator=(NetFlowTunnel&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock(mu_);
            node_ = std::move(other.node_);
            mode_ = other.mode_;
            active_.store(other.active_.load());
        }
        return *this;
    }

    void activate() {
        std::lock_guard<std::mutex> lock(mu_);
        active_ = true;
        node_.lastSeen = std::chrono::system_clock::now();
    }

    void deactivate() {
        std::lock_guard<std::mutex> lock(mu_);
        active_ = false;
    }

    bool isActive() const { return active_.load(); }

    TunnelMode mode() const { return mode_; }
    RelayNode node() const { return node_; }

    // Relay bandwidth with optional tunnel-mode penalty (implemented in AILEE_NetFlow.cpp).
    double relayBandwidth(double requestedMbps);

private:
    RelayNode node_;
    TunnelMode mode_;
    mutable std::mutex mu_;
    std::atomic<bool> active_;
};

// ----------------- Mesh Coordinator -----------------

class NetFlowMesh {
public:
    void registerNode(const RelayNode& node) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_[node.id.pubkey] = node;
    }

    void removeNode(const std::string& pubkey) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_.erase(pubkey);
    }

    std::optional<RelayNode> selectNode(double minBandwidthMbps = 1.0) {
        std::lock_guard<std::mutex> lock(mu_);
        RelayNode* best = nullptr;
        double bestScore = -1.0;
        for (auto& [k, n] : nodes_) {
            if (!n.online) continue;
            if (n.advertisedBandwidthMbps < minBandwidthMbps) continue;
            double score = n.advertisedBandwidthMbps; // could include latency, ZK audit etc.
            if (score > bestScore) { bestScore = score; best = &n; }
        }
        if (best) return *best;
        return std::nullopt;
    }

    std::vector<RelayNode> allNodes() const {
        std::lock_guard<std::mutex> lock(mu_);
        std::vector<RelayNode> vec;
        for (auto& [_, n] : nodes_) vec.push_back(n);
        return vec;
    }

    void refillAll(double refillMbps) {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& [_, node] : nodes_) {
            node.advertisedBandwidthMbps += refillMbps;
            node.online = true;
            node.lastSeen = std::chrono::system_clock::now();
            markNodeOnline(node);
        }
    }

    // Token reward for providing bandwidth
    TokenizedBandwidth rewardNode(const RelayNode& node, double bandwidthUsed, double baseRate) {
        TokenizedBandwidth t;
        t.nodePubkey = node.id.pubkey;
        t.tokensEarned = bandwidthUsed * baseRate;
        t.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        ailee::zk::ZKEngine zkEngine;
        auto proof = zkEngine.generateProof(node.id.pubkey, std::to_string(t.tokensEarned));
        t.zkProofHash = proof.proofData;
        return t;
    }

private:
    void markNodeOnline(const RelayNode& /*node*/) const {}

    std::unordered_map<std::string, RelayNode> nodes_;
    mutable std::mutex mu_;
};

// ----------------- Hybrid Tunnel Logic -----------------

class HybridNetFlow {
public:
    void addTunnel(const NetFlowTunnel& tunnel) {
        std::lock_guard<std::mutex> lock(mu_);
        tunnels_.push_back(tunnel);
    }

    double pushTraffic(double requestedMbps) {
        std::lock_guard<std::mutex> lock(mu_);
        double remaining = requestedMbps;
        for (auto& t : tunnels_) {
            if (!t.isActive()) continue;
            double allocated = t.relayBandwidth(remaining);
            remaining -= allocated;
            if (remaining <= 0) break;
        }
        return requestedMbps - remaining;
    }

    std::vector<NetFlowTunnel> activeTunnels() const {
        std::lock_guard<std::mutex> lock(mu_);
        std::vector<NetFlowTunnel> active;
        for (auto& t : tunnels_) if (t.isActive()) active.push_back(t);
        return active;
    }

private:
    std::vector<NetFlowTunnel> tunnels_;
    mutable std::mutex mu_;
};

} // namespace ailee_netflow
