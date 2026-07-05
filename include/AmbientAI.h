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

// ================= Constants =================
constexpr uint64_t FIXED_POINT_SCALE = 10000;

// ================= Core Data Models =================

struct NodeId {
    std::string pubkey;       // Verifiable identity
    std::string region;       // Geo/cluster tag
    std::string deviceClass;  // e.g., "smartphone", "gateway", "miner"
};

struct EnergyProfile {
    uint64_t inputPowerWFp = 0;
    uint64_t wasteHeatRecoveredWFp = 0;
    uint64_t temperatureCFp = 0;
    uint64_t ambientTempCFp = 0;
    uint64_t carbonIntensity_gCO2_kWhFp = 0;
    uint64_t computeEfficiency_GFLOPS_WFp = 0; // Efficiency metric
};

struct ComputeProfile {
    uint64_t cpuUtilizationFp = 0;
    uint64_t npuUtilizationFp = 0;
    uint64_t gpuUtilizationFp = 0;
    uint64_t availableMemMBFp = 0;
    uint64_t bandwidthMbpsFp = 0;
    uint64_t latencyMsFp = 0;
    uint64_t instantaneousPower_GFLOPSFp = 0; // For reward calculations
};

struct PrivacyBudget {
    uint64_t epsilonFp = FIXED_POINT_SCALE; // defaults to 1.0 scaled
    uint64_t deltaFp  = 0; // scaled
    uint64_t privacyBudgetRemainingFp = FIXED_POINT_SCALE; // 100% scaled
    bool homomorphicEncryptionEnabled = false;
    bool zeroKnowledgeProofEnabled = false;
};

struct TelemetrySample {
    NodeId node;
    EnergyProfile energy;
    ComputeProfile compute;
    uint64_t protocolTimestampMs = 0;
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
    uint64_t rewardTokensFp = 0;
    bool distributed = false;
};

struct Reputation {
    NodeId node;
    uint64_t scoreFp = 0;
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
        uint64_t lastActivityMs = 0;
        std::vector<std::string> activityLog;
    };

    explicit LocalSessionManager(const std::string& nodeId, uint64_t currentTimestampMs = 0) {
        state_.nodeId = nodeId;
        state_.lastActivityMs = currentTimestampMs;
    }

    void recordActivity(const std::string& event, uint64_t currentTimestampMs) {
        std::lock_guard<std::mutex> lock(mu_);
        state_.lastActivityMs = currentTimestampMs;
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
    void runMaintenanceTick(uint64_t currentTimestampMs) {
        std::lock_guard<std::mutex> lock(mu_);
        state_.sessionToken++;
        state_.lastActivityMs = currentTimestampMs;
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
    uint64_t maxTemperatureCFp = 80 * FIXED_POINT_SCALE;
    uint64_t maxLatencyMsFp    = 300 * FIXED_POINT_SCALE;
    uint64_t maxBlockMBFp      = 8 * FIXED_POINT_SCALE;
    int      maxErrorCount     = 25;
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
    explicit AmbientNode(NodeId id, SafetyPolicy policy, std::shared_ptr<ailee::l1::ReorgDetector> db = nullptr, uint64_t creationTimestampMs = 0)
        : id_(std::move(id)), policy_(policy),
          sessionManager_(std::make_shared<LocalSessionManager>(id_.pubkey, creationTimestampMs)),
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
        safeMode_.store(sample.energy.temperatureCFp > policy_.maxTemperatureCFp ||
                        sample.compute.latencyMsFp > policy_.maxLatencyMsFp);

        // Event logging
        if (safeMode_.load()) {
            std::cout << "[SAFE MODE] Node " << id_.pubkey
                      << " triggered safe mode.\n";
        }

        // Generate ZK proof automatically
        ailee::zk::ZKEngine zkEngine;
        auto proof = zkEngine.generateProof(id_.pubkey,
                                            std::to_string(sample.compute.cpuUtilizationFp));
        lastProof_ = { "telemetry_circuit", proof.proofData,
                       zkEngine.verifyProof(proof), sample.protocolTimestampMs };
    }

    FederatedUpdate runLocalTraining(const ailee::fl::FLTask& task,
                                     const std::vector<uint8_t>& deltaBytes,
                                     std::size_t numSamplesTrained,
                                     uint64_t trainingLossFp,
                                     uint64_t computeTimeMs,
                                     uint64_t protocolTimestampMs);

    ZKProofStub verifyComputation(const std::string& taskId,
                                  const std::string& circuitId,
                                  const std::string& resultHash,
                                  uint64_t protocolTimestampMs) {
        ailee::zk::ZKEngine zkEngine;
        auto proof = zkEngine.generateProof(taskId, resultHash);

        ZKProofStub p;
        p.circuitId = circuitId;
        p.proofHash = proof.proofData;
        p.verified  = zkEngine.verifyProof(proof);
        p.timestampMs = protocolTimestampMs;

        lastProof_ = p;
        return p;
    }

    IncentiveRecord accrueReward(const std::string& taskId, uint64_t rewardTokensFp) const {
        IncentiveRecord rec;
        rec.taskId = taskId;
        rec.node = id_;
        rec.rewardTokensFp = rewardTokensFp;
        rec.distributed = false;
        return rec;
    }

    void updateReputation(bool success, uint64_t deltaScoreFp) {
        std::lock_guard<std::mutex> lock(mu_);
        if (success) {
            rep_.completedTasks++;
            rep_.scoreFp += deltaScoreFp;
            // Cap at 1.0 scaled
            if (rep_.scoreFp > FIXED_POINT_SCALE) {
                rep_.scoreFp = FIXED_POINT_SCALE;
            }
        } else {
            rep_.disputes++;
            if (rep_.scoreFp >= deltaScoreFp) {
                rep_.scoreFp -= deltaScoreFp;
            } else {
                rep_.scoreFp = 0;
            }
        }

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
        try {
            auto j = nlohmann::json::parse(data);
            if (j.contains("scoreFp") && j["scoreFp"].is_number()) {
                rep_.scoreFp = j["scoreFp"].get<uint64_t>();
            } else if (j.contains("score") && j["score"].is_number()) {
                // Migrate from double score
                rep_.scoreFp = static_cast<uint64_t>(j["score"].get<double>() * FIXED_POINT_SCALE);
            } else {
                rep_.scoreFp = FIXED_POINT_SCALE; // default to 1.0 scaled
            }
        } catch (...) {
            rep_.scoreFp = FIXED_POINT_SCALE;
        }
    }
    std::string toJson() const { return "{\"scoreFp\": " + std::to_string(rep_.scoreFp) + "}"; }
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
    int64_t healthScoreFp() const {
        std::lock_guard<std::mutex> lock(mu_);
        if (!lastSample_.has_value()) return 0;

        const auto& s = lastSample_.value();
        int64_t scoreFp = 0;
        scoreFp += (s.compute.bandwidthMbpsFp * 4) / 10;      // bandwidth weight 0.4
        scoreFp -= (s.compute.latencyMsFp * 3) / 10;         // latency penalty 0.3
        scoreFp += (s.energy.computeEfficiency_GFLOPS_WFp * 2) / 10; // compute efficiency 0.2
        scoreFp += (rep_.scoreFp * 1) / 10;                   // reputation weight 0.1

        if (safeMode_.load()) {
            scoreFp = scoreFp / 2; // safe mode penalty 0.5
        }

        return std::max<int64_t>(0, scoreFp);
    }

private:
    NodeId id_;
    SafetyPolicy policy_;
    mutable std::mutex mu_;
    std::optional<TelemetrySample> lastSample_;
    ZKProofStub lastProof_;
    Reputation rep_{id_, 0, 0, 0};
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
        int64_t bestScoreFp = -1;

        for (auto* n : nodes_) {
            if (n->isSafeMode()) continue;

            auto last = n->last();
            if (!last.has_value()) continue;

            if (requireValidProof) {
                auto proof = n->lastProof();
                if (!proof.has_value() || !proof->verified) continue;
            }

            int64_t scoreFp = n->healthScoreFp();
            if (scoreFp > bestScoreFp) { bestScoreFp = scoreFp; best = n; }
        }
        return best;
    }

    template<typename TaskFn>
    IncentiveRecord dispatchAndReward(const std::string& taskId, TaskFn fn, uint64_t baseRewardTokensFp) {
        AmbientNode* n = selectNodeForTask();
        if (!n) return IncentiveRecord{taskId, NodeId{"", "", ""}, 0, false};

        uint64_t multiplierFp = fn(*n);
        uint64_t rewardFp = (baseRewardTokensFp * multiplierFp) / FIXED_POINT_SCALE;

        // Scale reward based on privacy compliance and compute efficiency
        auto lastTelemetry = n->last();
        if (lastTelemetry.has_value()) {
            const auto& s = lastTelemetry.value();
            rewardFp = (rewardFp * s.energy.computeEfficiency_GFLOPS_WFp) / FIXED_POINT_SCALE;

            uint64_t privacyRemainingFp = s.privacy.privacyBudgetRemainingFp;
            if (privacyRemainingFp > FIXED_POINT_SCALE) privacyRemainingFp = FIXED_POINT_SCALE;

            rewardFp = (rewardFp * privacyRemainingFp) / FIXED_POINT_SCALE;
        }

        return n->accrueReward(taskId, rewardFp);
    }

    std::string clusterId() const { return clusterId_; }

private:
    std::string clusterId_;
    mutable std::mutex mu_;
    std::vector<AmbientNode*> nodes_;
};

} // namespace ambient
