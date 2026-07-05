// SPDX-License-Identifier: MIT
// AmbientAI.h — Bulletproof Ambient AI Node Interfaces for AILEE-Core
// Full integration of telemetry, ZK proofs, federated learning, token incentives,
// Byzantine detection, safety policies, and weighted node health scoring.

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <atomic>
#include <functional>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "FederatedLearning.h"
#include "zk_proofs.h"
#include "ReorgDetector.h" // ZK proof module

namespace ambient {

// ================= Core Data Models =================

struct NodeId {
    std::string pubkey;       // Verifiable identity
    std::string region;       // Geo/cluster tag
    std::string deviceClass;  // e.g., "smartphone", "gateway", "miner"
};

struct EnergyProfile {
    double inputPowerW = 0.0;
    double wasteHeatRecoveredW = 0.0;
    double temperatureC = 0.0;
    double ambientTempC = 0.0;
    double carbonIntensity_gCO2_kWh = 0.0;
    double computeEfficiency_GFLOPS_W = 0.0; // Efficiency metric
};

struct ComputeProfile {
    double cpuUtilization = 0.0;
    double npuUtilization = 0.0;
    double gpuUtilization = 0.0;
    double availableMemMB = 0.0;
    double bandwidthMbps = 0.0;
    double latencyMs = 0.0;
    double instantaneousPower_GFLOPS = 0.0; // For reward calculations
};

struct PrivacyBudget {
    double epsilon = 1.0;
    double delta  = 1e-5;
    double privacyBudgetRemaining = 1.0; // %
    bool homomorphicEncryptionEnabled = false;
    bool zeroKnowledgeProofEnabled = false;
};

struct TelemetrySample {
    NodeId node;
    EnergyProfile energy;
    ComputeProfile compute;
    std::chrono::system_clock::time_point timestamp;
    PrivacyBudget privacy;
    std::string cryptographicVerificationHash;
};

// ================= Federated Learning =================

using FederatedUpdate = ailee::fl::LocalDelta;

// ================= ZK Proof Integration =================

struct ZKProofStub {
    std::string proofHash;
    std::string circuitId;
    bool verified = false;
    uint64_t timestampMs = 0;
};

// ================= Token Incentives & Reputation =================

struct IncentiveRecord {
    std::string taskId;
    NodeId node;
    double rewardTokens = 0.0;
    bool distributed = false;
};

struct Reputation {
    NodeId node;
    double score = 0.0;
    uint64_t completedTasks = 0;
    uint64_t disputes = 0;
};

// ================= Local Session Manager =================
// Manages node session state locally, keeping the node "alive" even when
// disconnected from the API endpoint.

class LocalSessionManager {
public:
    struct SessionState {
        std::string nodeId;
        bool connected = false;
        uint64_t sessionToken = 0;
        std::chrono::system_clock::time_point lastActivity;
        std::vector<std::string> activityLog;
    };

    explicit LocalSessionManager(const std::string& nodeId) {
        state_.nodeId = nodeId;
        state_.lastActivity = std::chrono::system_clock::now();
    }

    void recordActivity(const std::string& event) {
        std::lock_guard<std::mutex> lock(mu_);
        state_.lastActivity = std::chrono::system_clock::now();
        state_.activityLog.push_back(event);
        if (state_.activityLog.size() > kMaxLogEntries) {
            state_.activityLog.erase(state_.activityLog.begin());
        }
    }

    void setConnected(bool connected) {
        std::lock_guard<std::mutex> lock(mu_);
        state_.connected = connected;
    }

    bool isConnected() const {
        std::lock_guard<std::mutex> lock(mu_);
        return state_.connected;
    }

    SessionState getState() const {
        std::lock_guard<std::mutex> lock(mu_);
        return state_;
    }

    // Maintenance tick: keeps session alive even when disconnected from the API endpoint.
    void runMaintenanceTick() {
        std::lock_guard<std::mutex> lock(mu_);
        state_.sessionToken++;
        state_.lastActivity = std::chrono::system_clock::now();
        if (!state_.connected) {
            state_.activityLog.push_back("[offline-keepalive] session maintained");
            if (state_.activityLog.size() > kMaxLogEntries) {
                state_.activityLog.erase(state_.activityLog.begin());
            }
        }
    }

private:
    static constexpr std::size_t kMaxLogEntries = 100;
    mutable std::mutex mu_;
    SessionState state_;
};

// ================= Safety / Circuit-Breaker Policy =================

struct SafetyPolicy {
    double maxTemperatureC = 80.0;
    double maxLatencyMs    = 300.0;
    double maxBlockMB      = 8.0;
    int    maxErrorCount   = 25;
};

// ================= Byzantine Behavior =================

enum class NodeBehavior {
    HONEST,
    BYZANTINE_SILENT,
    BYZANTINE_CORRUPT,
    BYZANTINE_SYBIL
};

// ================= AmbientNode Class =================

class AmbientNode {
public:
    explicit AmbientNode(NodeId id, SafetyPolicy policy, std::shared_ptr<ailee::l1::ReorgDetector> db = nullptr)
        : id_(std::move(id)), policy_(policy),
          sessionManager_(std::make_shared<LocalSessionManager>(id_.pubkey)),
          db_(db) {
              if (db_) {
                  auto repData = db_->getReputation(id_.pubkey);
                  if (repData) {
                      loadReputation(*repData);
                  }
              }
          }

    void ingestTelemetry(const TelemetrySample& sample) {
        std::lock_guard<std::mutex> lock(mu_);
        lastSample_ = sample;

        // Safe-mode toggle
        safeMode_.store(sample.energy.temperatureC > policy_.maxTemperatureC ||
                        sample.compute.latencyMs > policy_.maxLatencyMs);

        // Event logging
        if (safeMode_.load()) {
            std::cout << "[SAFE MODE] Node " << id_.pubkey
                      << " triggered safe mode.\n";
        }

        // Generate ZK proof automatically
        ailee::zk::ZKEngine zkEngine;
        auto proof = zkEngine.generateProof(id_.pubkey,
                                            std::to_string(sample.compute.cpuUtilization));
        lastProof_ = { "telemetry_circuit", proof.proofData,
                       zkEngine.verifyProof(proof), static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) };
    }

    FederatedUpdate runLocalTraining(const ailee::fl::FLTask& task,
                                     const std::vector<uint8_t>& deltaBytes,
                                     std::size_t numSamplesTrained,
                                     double trainingLoss,
                                     std::chrono::milliseconds computeTime);

    ZKProofStub verifyComputation(const std::string& taskId,
                                  const std::string& circuitId,
                                  const std::string& resultHash) {
        ailee::zk::ZKEngine zkEngine;
        auto proof = zkEngine.generateProof(taskId, resultHash);

        ZKProofStub p;
        p.circuitId = circuitId;
        p.proofHash = proof.proofData;
        p.verified  = zkEngine.verifyProof(proof);
        p.timestampMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

        lastProof_ = p;
        return p;
    }

    IncentiveRecord accrueReward(const std::string& taskId, double tokens) const {
        IncentiveRecord rec;
        rec.taskId = taskId;
        rec.node = id_;
        rec.rewardTokens = tokens;
        rec.distributed = false;
        return rec;
    }

    void updateReputation(bool success, double deltaScore) {
        std::lock_guard<std::mutex> lock(mu_);
        if (success) {
            rep_.completedTasks++;
            rep_.score += deltaScore;
        } else {
            rep_.disputes++;
            rep_.score -= deltaScore;
        }
        rep_.score = std::max(0.0, rep_.score);

        if (db_) {
            std::string repData = toJson();
            db_->setReputation(id_.pubkey, repData);
        }
    }

    bool isSafeMode() const { return safeMode_.load(); }

    NodeId id() const { return id_; }

    // Returns a reference to the local session manager for this node.
    // The session manager is intentionally excluded from serialization (see to_json).
    LocalSessionManager& sessionManager() { return *sessionManager_; }
    const LocalSessionManager& sessionManager() const { return *sessionManager_; }

    void loadReputation(const std::string& data) {

        std::lock_guard<std::mutex> lock(mu_);
        try { auto j = nlohmann::json::parse(data); if (j.contains("score") && j["score"].is_number()) { rep_.score = j["score"].get<double>(); } else { rep_.score = 100.0; } } catch (...) { rep_.score = 100.0; }
    }
    std::string toJson() const { return "{\"score\": " + std::to_string(rep_.score) + "}"; }
    Reputation reputation() const {
        std::lock_guard<std::mutex> lock(mu_);
        return rep_;
    }

    std::optional<TelemetrySample> last() const {
        std::lock_guard<std::mutex> lock(mu_);
        return lastSample_;
    }

    std::optional<ZKProofStub> lastProof() const {
        std::lock_guard<std::mutex> lock(mu_);
        return lastProof_;
    }

    // ================= Health Scoring =================
    double healthScore() const {
        std::lock_guard<std::mutex> lock(mu_);
        if (!lastSample_.has_value()) return 0.0;

        const auto& s = lastSample_.value();
        double score = 0.0;
        score += s.compute.bandwidthMbps * 0.4;      // bandwidth
        score += -s.compute.latencyMs * 0.3;         // latency penalty
        score += s.energy.computeEfficiency_GFLOPS_W * 0.2; // compute efficiency
        score += rep_.score * 0.1;                   // reputation weight
        if (safeMode_.load()) score *= 0.5;          // safe mode penalty
        return std::max(0.0, score);
    }

private:
    NodeId id_;
    SafetyPolicy policy_;
    mutable std::mutex mu_;
    std::optional<TelemetrySample> lastSample_;
    ZKProofStub lastProof_;
    Reputation rep_{id_, 0.0, 0, 0};
    std::atomic<bool> safeMode_{false};
    // sessionManager_ is excluded from serialization to avoid
    // capturing transient local state in snapshots or wire formats.
    std::shared_ptr<LocalSessionManager> sessionManager_;
    std::shared_ptr<ailee::l1::ReorgDetector> db_;
};

// ================= MeshCoordinator Class =================

class MeshCoordinator {
public:
    explicit MeshCoordinator(std::string clusterId)
        : clusterId_(std::move(clusterId)) {}

    void registerNode(AmbientNode* node) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_.push_back(node);
    }

    AmbientNode* selectNodeForTask(bool requireValidProof = true) {
        std::lock_guard<std::mutex> lock(mu_);
        AmbientNode* best = nullptr;
        double bestScore = -1.0;

        for (auto* n : nodes_) {
            if (n->isSafeMode()) continue;

            auto last = n->last();
            if (!last.has_value()) continue;

            if (requireValidProof) {
                auto proof = n->lastProof();
                if (!proof.has_value() || !proof->verified) continue;
            }

            double score = n->healthScore();
            if (score > bestScore) { bestScore = score; best = n; }
        }
        return best;
    }

    template<typename TaskFn>
    IncentiveRecord dispatchAndReward(const std::string& taskId, TaskFn fn, double baseRewardTokens) {
        AmbientNode* n = selectNodeForTask();
        if (!n) return IncentiveRecord{taskId, NodeId{"", "", ""}, 0.0, false};

        double multiplier = fn(*n);
        double reward = baseRewardTokens * multiplier;

        // Scale reward based on privacy compliance and compute efficiency
        auto lastTelemetry = n->last();
        if (lastTelemetry.has_value()) {
            const auto& s = lastTelemetry.value();
            reward *= s.energy.computeEfficiency_GFLOPS_W;
            reward *= std::clamp(s.privacy.privacyBudgetRemaining, 0.0, 1.0);
        }

        return n->accrueReward(taskId, reward);
    }

    std::string clusterId() const { return clusterId_; }

private:
    std::string clusterId_;
    mutable std::mutex mu_;
    std::vector<AmbientNode*> nodes_;
};

} // namespace ambient
