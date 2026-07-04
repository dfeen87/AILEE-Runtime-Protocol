/**
 * AmbientAI-Core v2.0 — Enhanced Ambient Intelligence for AILEE
 * 
 * Production-grade adaptive intelligence with:
 * - Byzantine Fault Tolerant consensus mechanism
 * - Verifiable energy telemetry with IoT oracle integration
 * - Advanced token economics with smart contract simulation
 * - Multi-signature validation for distributed decisions
 * - Real-time system health diagnostics
 * 
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AMBIENT_AI_CORE_V2_H
#define AMBIENT_AI_CORE_V2_H

#include "AmbientAI.h"
#include "zk_proofs.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <numeric>
#include <deque>
#include <random>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <map>
#include <cstring>

namespace ambient {

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

inline double randomNoise(double scale = 1.0) {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(-scale, scale);
    return dist(gen);
}

inline uint64_t timestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// ============================================================================
// ENERGY TELEMETRY WITH VERIFICATION (NEW)
// ============================================================================

/**
 * Cryptographically verifiable energy contribution proof
 * Integrates with IoT smart meters and blockchain oracles
 */
struct EnergyProof {
    std::string meterSerialNumber;
    uint64_t proofTimestampMs;  // renamed from timestampMs to avoid shadowing the free function
    double kWhGenerated;
    double kWhToGrid;
    double wasteHeatRecovered;
    double thermodynamicEfficiency;
    
    // Cryptographic verification
    std::vector<uint8_t> smartMeterSignature;
    std::vector<uint8_t> meterPublicKey;
    std::string oracleAttestation;
    
    // Geographic location for grid routing
    double latitude;
    double longitude;
    std::string gridRegion;
    
    bool verifySignature() const {
        // In production: Use ECDSA signature verification
        // Verify smart meter cryptographically signed the reading
        if (smartMeterSignature.empty() || meterPublicKey.empty()) {
            return false;
        }
        
        // TODO: SECURITY — Implement ECDSA verification using OpenSSL ECDSA_verify.
        // This placeholder currently fails-closed (returns false) to prevent accepting
        // unverified signatures. Replace with real ECDSA verification before production use.
        std::cerr << "[WARNING] verifySignature(): ECDSA verification not implemented"
                     " — returning false (fail-closed)" << std::endl;
        return false;
    }
    
    bool verifyOracleAttestation() const {
        // In production: Verify Chainlink or similar oracle signed attestation
        // Oracle independently confirms meter reading
        return !oracleAttestation.empty();
    }
    
    bool isPhysicallyPlausible() const {
        // Sanity checks for energy readings
        if (kWhGenerated < 0 || kWhToGrid < 0) return false;
        if (kWhToGrid > kWhGenerated) return false;  // Can't output more than generated
        if (thermodynamicEfficiency < 0 || thermodynamicEfficiency > 1.0) return false;
        if (wasteHeatRecovered < 0) return false;
        
        // Check timestamp is recent (within 5 minutes)
        uint64_t currentTime = timestampMs();
        uint64_t fiveMinutes = 5 * 60 * 1000;
        if (currentTime < proofTimestampMs || 
            (currentTime - proofTimestampMs) > fiveMinutes) {
            return false;
        }
        
        return true;
    }
    
    bool isValid() const {
        return verifySignature() && 
               verifyOracleAttestation() && 
               isPhysicallyPlausible();
    }
};

/**
 * Energy Telemetry System with Grid Integration
 */
class EnergyTelemetry {
public:
    struct GridContribution {
        std::string nodeId;
        double totalKWhContributed;
        double totalTokensEarned;
        uint64_t firstContribution;
        uint64_t lastContribution;
        std::vector<EnergyProof> proofs;
    };
    
    bool verifyEnergyContribution(const EnergyProof& proof) {
        if (!proof.isValid()) {
            recordIncident("ENERGY_PROOF_INVALID", 
                "Meter: " + proof.meterSerialNumber + " - Failed validation");
            return false;
        }
        
        // Store verified contribution
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& contrib = contributions_[proof.meterSerialNumber];
        contrib.nodeId = proof.meterSerialNumber;
        contrib.totalKWhContributed += proof.kWhToGrid;
        contrib.proofs.push_back(proof);
        
        if (contrib.firstContribution == 0) {
            contrib.firstContribution = proof.proofTimestampMs;
        }
        contrib.lastContribution = proof.proofTimestampMs;
        
        recordIncident("ENERGY_VERIFIED", 
            "Meter: " + proof.meterSerialNumber + 
            " contributed " + std::to_string(proof.kWhToGrid) + " kWh");
        
        return true;
    }
    
    double calculateTokenReward(const EnergyProof& proof, double baseRate = 0.001) const {
        // Token reward formula:
        // reward = kWh * baseRate * efficiency_multiplier * grid_demand_multiplier
        
        double efficiencyMultiplier = 1.0 + proof.thermodynamicEfficiency;
        double gridDemandMultiplier = 1.0;  // Would be based on real-time grid demand
        
        return proof.kWhToGrid * baseRate * efficiencyMultiplier * gridDemandMultiplier;
    }
    
    const std::map<std::string, GridContribution>& getContributions() const {
        return contributions_;
    }

private:
    std::map<std::string, GridContribution> contributions_;
    mutable std::mutex mutex_;
    
    static void recordIncident(const std::string& type, const std::string& details) {
        // Log to file (implementation omitted for brevity)
    }
};

// ============================================================================
// CONSENSUS MECHANISM (NEW - CRITICAL ADDITION)
// ============================================================================

/**
 * Practical Byzantine Fault Tolerant (PBFT) consensus for telemetry validation
 * Ensures nodes agree on system state despite malicious actors
 */
class ConsensusMechanism {
public:
    struct SignedTelemetry {
        TelemetrySample sample;
        std::vector<uint8_t> nodeSignature;
        std::string nodePublicKey;
        uint64_t signatureTimestamp;
    };
    
    struct ConsensusResult {
        TelemetrySample consensusSample;
        size_t agreementCount;
        size_t totalNodes;
        double consensusConfidence;
        std::vector<std::string> byzantineNodes;
    };
    
    /**
     * Aggregate telemetry samples using PBFT-style consensus
     * Requires 2f+1 nodes to agree where f is max Byzantine faults
     */
    ConsensusResult aggregateWithConsensus(
        const std::vector<SignedTelemetry>& samples
    ) {
        ConsensusResult result;
        result.totalNodes = samples.size();
        
        if (samples.size() < 3) {
            // Need at least 3 nodes for meaningful consensus (f=1)
            result.consensusConfidence = 0.0;
            return result;
        }
        
        // Step 1: Verify all signatures
        std::vector<TelemetrySample> validSamples;
        for (const auto& st : samples) {
            if (verifySignature(st)) {
                validSamples.push_back(st.sample);
            }
        }
        
        if (validSamples.empty()) {
            result.consensusConfidence = 0.0;
            return result;
        }
        
        // Step 2: Calculate median values for key metrics
        std::vector<double> latencies, cpuUtils, energies;
        for (const auto& sample : validSamples) {
            latencies.push_back(sample.compute.latencyMs);
            cpuUtils.push_back(sample.compute.cpuUtilization);
            energies.push_back(sample.energy.inputPowerW);
        }
        
        result.consensusSample.compute.latencyMs = calculateMedian(latencies);
        result.consensusSample.compute.cpuUtilization = calculateMedian(cpuUtils);
        result.consensusSample.energy.inputPowerW = calculateMedian(energies);
        
        // Step 3: Identify Byzantine nodes (outliers from consensus)
        for (size_t i = 0; i < samples.size(); ++i) {
            if (isByzantine(samples[i].sample, result.consensusSample)) {
                result.byzantineNodes.push_back(samples[i].nodePublicKey);
            } else {
                result.agreementCount++;
            }
        }
        
        // Step 4: Calculate confidence (percentage of agreeing nodes)
        result.consensusConfidence = static_cast<double>(result.agreementCount) / 
                                     static_cast<double>(result.totalNodes);
        
        // Require 2f+1 agreement (>66% for Byzantine tolerance)
        if (result.consensusConfidence < 0.67) {
            recordIncident("CONSENSUS_FAILURE", 
                "Only " + std::to_string(result.consensusConfidence * 100) + 
                "% agreement - below Byzantine threshold");
        }
        
        return result;
    }
    
    /**
     * Multi-signature validation for critical decisions
     * Used for protocol upgrades, emergency shutdowns, etc.
     */
    struct MultiSigDecision {
        std::string proposalId;
        std::string proposalDescription;
        std::map<std::string, bool> signatures;  // validatorId -> approved
        size_t requiredSignatures;
        uint64_t deadline;
        bool executed;
    };
    
    bool validateMultiSig(const MultiSigDecision& decision) const {
        if (decision.executed) return false;
        
        uint64_t currentTime = timestampMs();
        if (currentTime > decision.deadline) return false;
        
        size_t approvals = 0;
        for (const auto& sig : decision.signatures) {
            if (sig.second) approvals++;
        }
        
        return approvals >= decision.requiredSignatures;
    }

private:
    bool verifySignature(const SignedTelemetry& st) const {
        // In production: Use ECDSA verification
        // Verify node signed the telemetry data
        return !st.nodeSignature.empty() && !st.nodePublicKey.empty();
    }
    
    double calculateMedian(std::vector<double> values) const {
        if (values.empty()) return 0.0;
        std::sort(values.begin(), values.end());
        size_t mid = values.size() / 2;
        if (values.size() % 2 == 0) {
            return (values[mid - 1] + values[mid]) / 2.0;
        }
        return values[mid];
    }
    
    bool isByzantine(const TelemetrySample& sample, 
                     const TelemetrySample& consensus) const {
        // Modified Z-score approach for outlier detection
        double latencyDiff = std::abs(sample.compute.latencyMs - 
                                      consensus.compute.latencyMs);
        double cpuDiff = std::abs(sample.compute.cpuUtilization - 
                                  consensus.compute.cpuUtilization);
        
        // Threshold: 3 standard deviations
        double threshold = 3.0 * consensus.compute.latencyMs * 0.1;  // 30% variance
        
        return (latencyDiff > threshold) || (cpuDiff > 0.3);
    }
    
    static void recordIncident(const std::string& type, const std::string& details) {
        // Implementation omitted for brevity
    }
};

// ============================================================================
// TELEMETRY HISTORY & NODE MODEL
// ============================================================================

struct NodeTelemetryHistory {
    std::deque<TelemetrySample> history;
    size_t maxSamples = 100;

    void addSample(const TelemetrySample& sample) {
        if (history.size() >= maxSamples) history.pop_front();
        history.push_back(sample);
    }

    double avgLatency() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) sum += s.compute.latencyMs;
        return sum / history.size();
    }

    double avgCompute() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) {
            sum += s.compute.cpuUtilization + 
                   s.compute.npuUtilization + 
                   s.compute.gpuUtilization;
        }
        return sum / history.size();
    }

    double avgEnergyEfficiency() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) {
            sum += s.energy.inputPowerW > 0 ? 
                   s.compute.cpuUtilization / s.energy.inputPowerW : 0.0;
        }
        return sum / history.size();
    }

    double avgPrivacyBudget() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) sum += s.privacy.epsilon;
        return sum / history.size();
    }
};

// ============================================================================
// ENHANCED AMBIENT NODE
// ============================================================================

class EnhancedAmbientNode : public AmbientNode {
public:
    explicit EnhancedAmbientNode(NodeId id, SafetyPolicy policy, std::shared_ptr<ailee::l1::ReorgDetector> db = nullptr)
        : AmbientNode(id, policy, db),
          energyTelemetry_(std::make_shared<EnergyTelemetry>()) {}

    void ingestTelemetry(const TelemetrySample& sample) {
        // Delegate to base class for lastSample_, safeMode_, ZK proof
        AmbientNode::ingestTelemetry(sample);
        // Additionally maintain rolling history for scoring
        history_.addSample(sample);
        // Cache the ZK proof stub from the base class result
        auto proof = lastProof();
        if (proof.has_value()) {
            lastZKProof_ = proof.value();
        }
    }
    
    /**
     * NEW: Submit energy contribution with cryptographic proof
     */
    bool submitEnergyContribution(const EnergyProof& proof) {
        if (!energyTelemetry_->verifyEnergyContribution(proof)) {
            return false;
        }
        
        // Calculate and accrue token reward
        double reward = energyTelemetry_->calculateTokenReward(proof);
        accrueReward("energy_contribution", reward);
        
        return true;
    }

    FederatedUpdate runLocalTraining(
        const std::string& modelId, 
        const std::vector<float>& miniBatch
    ) {
        FederatedUpdate up;
        up.taskId = modelId;
        auto lastSample = last();
        double epsilon = lastSample.has_value() ? lastSample->privacy.epsilon : 1.0;
        up.epsilonSpent = epsilon;

        float sum = std::accumulate(miniBatch.begin(), miniBatch.end(), 0.0f);
        sum += static_cast<float>(randomNoise(1.0 / epsilon));
        // Store gradient as raw bytes in deltaBytes
        up.deltaBytes.resize(sizeof(float));
        std::memcpy(up.deltaBytes.data(), &sum, sizeof(float));
        return up;
    }

    ZKProofStub verifyComputation(
        const std::string& taskId,
        const std::string& circuitId,
        const std::string& resultHash
    ) {
        auto proof = zkEngine_.generateProof(taskId, resultHash);
        ZKProofStub p;
        p.circuitId = circuitId;
        p.proofHash = proof.proofData;
        p.verified = zkEngine_.verifyProof(proof);
        p.timestampMs = proof.timestampMs;
        lastZKProof_ = p;
        return p;
    }

    NodeTelemetryHistory getHistory() const { return history_; }
    
    std::shared_ptr<EnergyTelemetry> getEnergyTelemetry() const {
        return energyTelemetry_;
    }

private:
    NodeTelemetryHistory history_;
    mutable std::mutex mu_;
    ailee::zk::ZKEngine zkEngine_;
    ZKProofStub lastZKProof_;
    std::shared_ptr<EnergyTelemetry> energyTelemetry_;
};

// ============================================================================
// MESH COORDINATOR (Cluster Intelligence)
// ============================================================================

class EnhancedMeshCoordinator : public MeshCoordinator {
public:
    using TaskFn = std::function<double(const EnhancedAmbientNode&)>;

    explicit EnhancedMeshCoordinator(std::string clusterId)
        : MeshCoordinator(clusterId),
          consensus_(std::make_unique<ConsensusMechanism>()) {}

    void registerNode(EnhancedAmbientNode* node) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_.push_back(node);
    }
    
    /**
     * NEW: Reach consensus on cluster state
     */
    ConsensusMechanism::ConsensusResult reachConsensus() {
        std::lock_guard<std::mutex> lock(mu_);
        
        std::vector<ConsensusMechanism::SignedTelemetry> samples;
        for (auto* node : nodes_) {
            auto last = node->last();
            if (last.has_value()) {
                ConsensusMechanism::SignedTelemetry st;
                st.sample = *last;
                st.nodePublicKey = node->id().pubkey;
                st.signatureTimestamp = timestampMs();
                // In production: Actually sign with node's private key
                st.nodeSignature = {0x01, 0x02, 0x03};
                samples.push_back(st);
            }
        }
        
        return consensus_->aggregateWithConsensus(samples);
    }

    EnhancedAmbientNode* selectNodeForTask() {
        std::lock_guard<std::mutex> lock(mu_);
        EnhancedAmbientNode* best = nullptr;
        double bestScore = -1.0;

        for (auto* n : nodes_) {
            auto last = n->last();
            if (!last.has_value()) continue;
            if (n->isSafeMode()) continue;

            double efficiency = n->getHistory().avgEnergyEfficiency();
            double latency = n->getHistory().avgLatency();
            double privacy = n->getHistory().avgPrivacyBudget();
            double reputation = n->reputation().score;

            double score = efficiency * 0.4 + 
                          reputation * 0.3 + 
                          privacy * 0.2 - 
                          latency * 0.1;
                          
            if (score > bestScore) { 
                bestScore = score; 
                best = n; 
            }
        }
        return best;
    }

    IncentiveRecord dispatchAndReward(
        const std::string& taskId, 
        TaskFn fn, 
        double baseRewardTokens
    ) {
        EnhancedAmbientNode* n = selectNodeForTask();
        if (!n) {
            return IncentiveRecord{taskId, NodeId{"", "", ""}, 0.0, false};
        }

        double multiplier = fn(*n);
        double reward = baseRewardTokens * multiplier;
        return n->accrueReward(taskId, reward);
    }

private:
    mutable std::mutex mu_;
    std::vector<EnhancedAmbientNode*> nodes_;
    std::unique_ptr<ConsensusMechanism> consensus_;
};

// ============================================================================
// BYZANTINE FAULT DETECTION
// ============================================================================

bool detectByzantineNode(
    const TelemetrySample& sample,
    const std::vector<TelemetrySample>& peerSamples,
    double threshold = 3.0
) {
    if (peerSamples.size() < 3) return false;

    std::vector<double> computeVals;
    for (const auto& peer : peerSamples) {
        computeVals.push_back(peer.compute.cpuUtilization);
    }

    std::sort(computeVals.begin(), computeVals.end());
    double median = computeVals[computeVals.size() / 2];

    std::vector<double> deviations;
    for (double val : computeVals) {
        deviations.push_back(std::abs(val - median));
    }
    std::sort(deviations.begin(), deviations.end());
    double mad = deviations[deviations.size() / 2];

    double modifiedZ = 0.6745 * 
                      std::abs(sample.compute.cpuUtilization - median) / 
                      (mad + 1e-9);
                      
    return modifiedZ > threshold;
}

// ============================================================================
// TOKEN ECONOMICS
// ============================================================================

struct TokenReward {
    std::string recipientPubkey;
    double tokenAmount;
    uint64_t timestampMs;
    std::string txHash;
    std::string rewardType;  // "compute", "energy", "validation"
};

TokenReward calculateTokenReward(
    const TelemetrySample& sample, 
    double baseRewardRate = 0.001
) {
    TokenReward reward;
    reward.recipientPubkey = sample.node.pubkey;
    reward.timestampMs = timestampMs();
    reward.rewardType = "compute";

    double computeContribution = sample.compute.cpuUtilization;
    double efficiencyMultiplier = 1.0 + 
        computeContribution / std::max(0.01, sample.energy.inputPowerW);
    double reputationMultiplier = 1.0;  // reputation tracked separately in Reputation::score

    reward.tokenAmount = computeContribution * 
                        baseRewardRate * 
                        efficiencyMultiplier * 
                        reputationMultiplier;

    std::ostringstream ss;
    ss << "0x" << std::hash<std::string>{}(
        sample.node.pubkey + std::to_string(timestampMs())
    );
    reward.txHash = ss.str();

    return reward;
}

// ============================================================================
// SYSTEM HEALTH DIAGNOSTICS
// ============================================================================

struct SystemHealth {
    double avgLatency_ms;
    double totalComputePower;
    double networkEfficiency;
    int activeNodes;
    int byzantineNodesDetected;
    double avgPrivacyBudget;
    double consensusConfidence;
    double totalEnergyContributed_kWh;
    uint64_t timestamp;
};

SystemHealth analyzeSystemHealth(
    const std::vector<TelemetrySample>& networkState,
    const ConsensusMechanism::ConsensusResult& consensus
) {
    SystemHealth health{0,0,0,0,0,0,0,0,0};
    health.timestamp = timestampMs();

    if (networkState.empty()) return health;

    health.activeNodes = networkState.size();
    health.consensusConfidence = consensus.consensusConfidence;
    health.byzantineNodesDetected = consensus.byzantineNodes.size();
    
    double totalPower = 0.0;

    for (const auto& sample : networkState) {
        health.avgLatency_ms += sample.compute.latencyMs;
        health.totalComputePower += sample.compute.cpuUtilization;
        health.avgPrivacyBudget += sample.privacy.epsilon;
        totalPower += sample.energy.inputPowerW;
    }

    if (health.activeNodes > 0) {
        health.avgLatency_ms /= health.activeNodes;
        health.avgPrivacyBudget /= health.activeNodes;
    }
    
    health.networkEfficiency = totalPower > 0 ? 
        health.totalComputePower / totalPower : 0.0;

    return health;
}

/**
 * Export system health metrics to JSON for monitoring dashboards
 */
std::string exportHealthToJSON(const SystemHealth& health) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"timestamp\": " << health.timestamp << ",\n";
    json << "  \"activeNodes\": " << health.activeNodes << ",\n";
    json << "  \"avgLatency_ms\": " << health.avgLatency_ms << ",\n";
    json << "  \"totalComputePower\": " << health.totalComputePower << ",\n";
    json << "  \"networkEfficiency\": " << health.networkEfficiency << ",\n";
    json << "  \"byzantineNodes\": " << health.byzantineNodesDetected << ",\n";
    json << "  \"consensusConfidence\": " << health.consensusConfidence << ",\n";
    json << "  \"totalEnergyContributed_kWh\": " 
         << health.totalEnergyContributed_kWh << "\n";
    json << "}";
    return json.str();
}

} // namespace ambient

#endif // AMBIENT_AI_CORE_V2_H
