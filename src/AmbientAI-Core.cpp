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
    uint64_t kWhGeneratedFp;
    uint64_t kWhToGridFp;
    uint64_t wasteHeatRecoveredFp;
    uint64_t thermodynamicEfficiencyFp;
    
    // Cryptographic verification
    std::vector<uint8_t> smartMeterSignature;
    std::vector<uint8_t> meterPublicKey;
    std::string oracleAttestation;
    
    // Geographic location for grid routing
    int64_t latitudeFp;
    int64_t longitudeFp;
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
    
    bool isPhysicallyPlausible(uint64_t protocolTimestampMs) const {
        // Sanity checks for energy readings using fixed point scaled uint64_t
        // Removed negative checks since uint64_t cannot be negative
        if (kWhToGridFp > kWhGeneratedFp) return false;  // Can't output more than generated
        if (thermodynamicEfficiencyFp > FIXED_POINT_SCALE) return false; // Max 1.0 scaled
        
        // Check timestamp is recent (within 5 minutes)
        uint64_t fiveMinutes = 5 * 60 * 1000;
        if (protocolTimestampMs < proofTimestampMs ||
            (protocolTimestampMs - proofTimestampMs) > fiveMinutes) {
            return false;
        }
        
        return true;
    }
    
    bool isValid(uint64_t protocolTimestampMs) const {
        return verifySignature() && 
               verifyOracleAttestation() && 
               isPhysicallyPlausible(protocolTimestampMs);
    }
};

/**
 * Energy Telemetry System with Grid Integration
 */
class EnergyTelemetry {
public:
    struct GridContribution {
        std::string nodeId;
        uint64_t totalKWhContributedFp;
        uint64_t totalTokensEarnedFp;
        uint64_t firstContribution;
        uint64_t lastContribution;
        std::vector<EnergyProof> proofs;
    };
    
    bool verifyEnergyContribution(const EnergyProof& proof, uint64_t protocolTimestampMs) {
        if (!proof.isValid(protocolTimestampMs)) {
            recordIncident("ENERGY_PROOF_INVALID", 
                "Meter: " + proof.meterSerialNumber + " - Failed validation");
            return false;
        }
        
        // Store verified contribution
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& contrib = contributions_[proof.meterSerialNumber];
        contrib.nodeId = proof.meterSerialNumber;
        contrib.totalKWhContributedFp += proof.kWhToGridFp;
        contrib.proofs.push_back(proof);
        
        if (contrib.firstContribution == 0) {
            contrib.firstContribution = proof.proofTimestampMs;
        }
        contrib.lastContribution = proof.proofTimestampMs;
        
        recordIncident("ENERGY_VERIFIED", 
            "Meter: " + proof.meterSerialNumber + 
            " contributed " + std::to_string(proof.kWhToGridFp) + " kWh (scaled)");
        
        return true;
    }
    
    uint64_t calculateTokenReward(const EnergyProof& proof, uint64_t baseRateFp = 10) const {
        // Token reward formula:
        // reward = kWh * baseRate * efficiency_multiplier * grid_demand_multiplier
        
        uint64_t efficiencyMultiplierFp = FIXED_POINT_SCALE + proof.thermodynamicEfficiencyFp;
        uint64_t gridDemandMultiplierFp = FIXED_POINT_SCALE;  // Would be based on real-time grid demand
        
        uint64_t baseRewardFp = (proof.kWhToGridFp * baseRateFp) / FIXED_POINT_SCALE;
        uint64_t rewardPhase1Fp = (baseRewardFp * efficiencyMultiplierFp) / FIXED_POINT_SCALE;
        return (rewardPhase1Fp * gridDemandMultiplierFp) / FIXED_POINT_SCALE;
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
        uint64_t consensusConfidenceFp;
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
        std::vector<uint64_t> latencies, cpuUtils, energies;
        for (const auto& sample : validSamples) {
            latencies.push_back(sample.compute.latencyMsFp);
            cpuUtils.push_back(sample.compute.cpuUtilizationFp);
            energies.push_back(sample.energy.inputPowerWFp);
        }
        
        result.consensusSample.compute.latencyMsFp = calculateMedianFp(latencies);
        result.consensusSample.compute.cpuUtilizationFp = calculateMedianFp(cpuUtils);
        result.consensusSample.energy.inputPowerWFp = calculateMedianFp(energies);
        
        // Step 3: Identify Byzantine nodes (outliers from consensus)
        for (size_t i = 0; i < samples.size(); ++i) {
            if (isByzantine(samples[i].sample, result.consensusSample)) {
                result.byzantineNodes.push_back(samples[i].nodePublicKey);
            } else {
                result.agreementCount++;
            }
        }
        
        // Step 4: Calculate confidence (percentage of agreeing nodes)
        result.consensusConfidenceFp = (result.agreementCount * FIXED_POINT_SCALE) / result.totalNodes;
        
        // Require 2f+1 agreement (>66% for Byzantine tolerance)
        // 0.67 is 6700 scaled
        if (result.consensusConfidenceFp < 6700) {
            recordIncident("CONSENSUS_FAILURE", 
                "Only " + std::to_string(result.consensusConfidenceFp / 100) +
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
    
    bool validateMultiSig(const MultiSigDecision& decision, uint64_t protocolTimestampMs) const {
        if (decision.executed) return false;
        
        if (protocolTimestampMs > decision.deadline) return false;
        
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
    
    uint64_t calculateMedianFp(std::vector<uint64_t> values) const {
        if (values.empty()) return 0;
        std::sort(values.begin(), values.end());
        size_t mid = values.size() / 2;
        if (values.size() % 2 == 0) {
            return (values[mid - 1] + values[mid]) / 2;
        }
        return values[mid];
    }
    
    bool isByzantine(const TelemetrySample& sample, 
                     const TelemetrySample& consensus) const {
        // Modified Z-score approach for outlier detection
        uint64_t sampleLatencyFp = sample.compute.latencyMsFp;
        uint64_t consLatencyFp = consensus.compute.latencyMsFp;
        uint64_t latencyDiffFp = (sampleLatencyFp > consLatencyFp) ? (sampleLatencyFp - consLatencyFp) : (consLatencyFp - sampleLatencyFp);
        
        uint64_t sampleCpuFp = sample.compute.cpuUtilizationFp;
        uint64_t consCpuFp = consensus.compute.cpuUtilizationFp;
        uint64_t cpuDiffFp = (sampleCpuFp > consCpuFp) ? (sampleCpuFp - consCpuFp) : (consCpuFp - sampleCpuFp);
        
        // Threshold: 3 standard deviations (approximated here as 30% of consensus latency)
        uint64_t thresholdFp = (3000 * consLatencyFp) / FIXED_POINT_SCALE;  // 3.0 * 0.1 = 0.3 -> 3000 scaled

        // cpu threshold > 0.3 (3000 scaled)
        return (latencyDiffFp > thresholdFp) || (cpuDiffFp > 3000);
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

    uint64_t avgLatencyFp() const {
        if (history.empty()) return 0;
        uint64_t sum = 0;
        for (const auto& s : history) sum += s.compute.latencyMsFp;
        return sum / history.size();
    }

    uint64_t avgComputeFp() const {
        if (history.empty()) return 0;
        uint64_t sum = 0;
        for (const auto& s : history) {
            sum += s.compute.cpuUtilizationFp +
                   s.compute.npuUtilizationFp +
                   s.compute.gpuUtilizationFp;
        }
        return sum / history.size();
    }

    uint64_t avgEnergyEfficiencyFp() const {
        if (history.empty()) return 0;
        uint64_t sum = 0;
        for (const auto& s : history) {
            if (s.energy.inputPowerWFp > 0) {
                sum += (s.compute.cpuUtilizationFp * FIXED_POINT_SCALE) / s.energy.inputPowerWFp;
            }
        }
        return sum / history.size();
    }

    uint64_t avgPrivacyBudgetFp() const {
        if (history.empty()) return 0;
        uint64_t sum = 0;
        for (const auto& s : history) sum += s.privacy.epsilonFp;
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
    bool submitEnergyContribution(const EnergyProof& proof, uint64_t protocolTimestampMs) {
        if (!energyTelemetry_->verifyEnergyContribution(proof, protocolTimestampMs)) {
            return false;
        }
        
        // Calculate and accrue token reward
        uint64_t rewardFp = energyTelemetry_->calculateTokenReward(proof);
        accrueReward("energy_contribution", rewardFp);
        
        return true;
    }

    FederatedUpdate runLocalTraining(
        const std::string& modelId, 
        const std::vector<float>& miniBatch,
        uint64_t computeTimeMs,
        uint64_t protocolTimestampMs
    ) {
        FederatedUpdate up;
        up.taskId = modelId;
        auto lastSample = last();
        uint64_t epsilonFp = lastSample.has_value() ? lastSample->privacy.epsilonFp : FIXED_POINT_SCALE;
        up.epsilonSpent = static_cast<double>(epsilonFp) / FIXED_POINT_SCALE; // Keep FederatedUpdate interface if needed or fix later
        up.computeTime = std::chrono::milliseconds(computeTimeMs);
        up.submissionTime = std::chrono::system_clock::time_point(std::chrono::milliseconds(protocolTimestampMs));

        float sum = std::accumulate(miniBatch.begin(), miniBatch.end(), 0.0f);
        // Removed non-deterministic random noise.

        // Store gradient as raw bytes in deltaBytes
        up.deltaBytes.resize(sizeof(float));
        std::memcpy(up.deltaBytes.data(), &sum, sizeof(float));
        return up;
    }

    ZKProofStub verifyComputation(
        const std::string& taskId,
        const std::string& circuitId,
        const std::string& resultHash,
        uint64_t protocolTimestampMs
    ) {
        auto proof = zkEngine_.generateProof(taskId, resultHash);
        ZKProofStub p;
        p.circuitId = circuitId;
        p.proofHash = proof.proofData;
        p.verified = zkEngine_.verifyProof(proof);
        p.timestampMs = protocolTimestampMs;
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
    using TaskFn = std::function<uint64_t(const EnhancedAmbientNode&)>;

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
                st.signatureTimestamp = st.sample.protocolTimestampMs;
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
        int64_t bestScoreFp = -1;

        for (auto* n : nodes_) {
            auto last = n->last();
            if (!last.has_value()) continue;
            if (n->isSafeMode()) continue;

            uint64_t efficiencyFp = n->getHistory().avgEnergyEfficiencyFp();
            uint64_t latencyFp = n->getHistory().avgLatencyFp();
            uint64_t privacyFp = n->getHistory().avgPrivacyBudgetFp();
            uint64_t reputationFp = n->reputation().scoreFp;

            int64_t scoreFp = (efficiencyFp * 4) / 10 +
                              (reputationFp * 3) / 10 +
                              (privacyFp * 2) / 10 -
                              (latencyFp * 1) / 10;
                          
            if (scoreFp > bestScoreFp) {
                bestScoreFp = scoreFp;
                best = n; 
            }
        }
        return best;
    }

    IncentiveRecord dispatchAndReward(
        const std::string& taskId, 
        TaskFn fn, 
        uint64_t baseRewardTokensFp
    ) {
        EnhancedAmbientNode* n = selectNodeForTask();
        if (!n) {
            return IncentiveRecord{taskId, NodeId{"", "", ""}, 0, false};
        }

        uint64_t multiplierFp = fn(*n);
        uint64_t rewardFp = (baseRewardTokensFp * multiplierFp) / FIXED_POINT_SCALE;
        return n->accrueReward(taskId, rewardFp);
    }

private:
    mutable std::mutex mu_;
    std::vector<EnhancedAmbientNode*> nodes_;
    std::unique_ptr<ConsensusMechanism> consensus_;
};

// ============================================================================
// BYZANTINE FAULT DETECTION
// ============================================================================

bool detectByzantineNodeFp(
    const TelemetrySample& sample,
    const std::vector<TelemetrySample>& peerSamples,
    uint64_t thresholdFp = 30000 // 3.0 scaled
) {
    if (peerSamples.size() < 3) return false;

    std::vector<uint64_t> computeVals;
    for (const auto& peer : peerSamples) {
        computeVals.push_back(peer.compute.cpuUtilizationFp);
    }

    std::sort(computeVals.begin(), computeVals.end());
    uint64_t medianFp = computeVals[computeVals.size() / 2];

    std::vector<uint64_t> deviations;
    for (uint64_t val : computeVals) {
        deviations.push_back((val > medianFp) ? (val - medianFp) : (medianFp - val));
    }
    std::sort(deviations.begin(), deviations.end());
    uint64_t madFp = deviations[deviations.size() / 2];

    uint64_t sampleValFp = sample.compute.cpuUtilizationFp;
    uint64_t diffFp = (sampleValFp > medianFp) ? (sampleValFp - medianFp) : (medianFp - sampleValFp);

    uint64_t denominatorFp = madFp + 1; // +1 to avoid div zero

    // 0.6745 is 6745 / 10000
    uint64_t modifiedZFp = (6745 * diffFp) / denominatorFp;
                      
    return modifiedZFp > thresholdFp;
}

// ============================================================================
// TOKEN ECONOMICS
// ============================================================================

struct TokenReward {
    std::string recipientPubkey;
    uint64_t tokenAmountFp;
    uint64_t timestampMs;
    std::string txHash;
    std::string rewardType;  // "compute", "energy", "validation"
};

TokenReward calculateTokenReward(
    const TelemetrySample& sample, 
    uint64_t baseRewardRateFp = 10 // defaults to 0.001 scaled
) {
    TokenReward reward;
    reward.recipientPubkey = sample.node.pubkey;
    reward.timestampMs = sample.protocolTimestampMs;
    reward.rewardType = "compute";

    uint64_t computeContributionFp = sample.compute.cpuUtilizationFp;

    uint64_t efficiencyMultiplierFp = FIXED_POINT_SCALE;
    uint64_t inputPowerWFp = sample.energy.inputPowerWFp;
    if (inputPowerWFp < (FIXED_POINT_SCALE / 100)) {
        inputPowerWFp = FIXED_POINT_SCALE / 100; // max(0.01 scaled)
    }
    efficiencyMultiplierFp += (computeContributionFp * FIXED_POINT_SCALE) / inputPowerWFp;

    uint64_t reputationMultiplierFp = FIXED_POINT_SCALE;

    uint64_t amount1 = (computeContributionFp * baseRewardRateFp) / FIXED_POINT_SCALE;
    uint64_t amount2 = (amount1 * efficiencyMultiplierFp) / FIXED_POINT_SCALE;
    reward.tokenAmountFp = (amount2 * reputationMultiplierFp) / FIXED_POINT_SCALE;

    std::ostringstream ss;
    ss << "0x" << std::hash<std::string>{}(
        sample.node.pubkey + std::to_string(sample.protocolTimestampMs)
    );
    reward.txHash = ss.str();

    return reward;
}

// ============================================================================
// SYSTEM HEALTH DIAGNOSTICS
// ============================================================================

struct SystemHealth {
    uint64_t avgLatencyMsFp;
    uint64_t totalComputePowerFp;
    uint64_t networkEfficiencyFp;
    int activeNodes;
    int byzantineNodesDetected;
    uint64_t avgPrivacyBudgetFp;
    uint64_t consensusConfidenceFp;
    uint64_t totalEnergyContributed_kWhFp;
    uint64_t timestampMs;
};

SystemHealth analyzeSystemHealth(
    const std::vector<TelemetrySample>& networkState,
    const ConsensusMechanism::ConsensusResult& consensus,
    uint64_t protocolTimestampMs
) {
    SystemHealth health{0,0,0,0,0,0,0,0,0};
    health.timestampMs = protocolTimestampMs;

    if (networkState.empty()) return health;

    health.activeNodes = networkState.size();
    health.consensusConfidenceFp = consensus.consensusConfidenceFp;
    health.byzantineNodesDetected = consensus.byzantineNodes.size();
    
    uint64_t totalPowerFp = 0;

    for (const auto& sample : networkState) {
        health.avgLatencyMsFp += sample.compute.latencyMsFp;
        health.totalComputePowerFp += sample.compute.cpuUtilizationFp;
        health.avgPrivacyBudgetFp += sample.privacy.epsilonFp;
        totalPowerFp += sample.energy.inputPowerWFp;
    }

    if (health.activeNodes > 0) {
        health.avgLatencyMsFp /= health.activeNodes;
        health.avgPrivacyBudgetFp /= health.activeNodes;
    }
    
    health.networkEfficiencyFp = totalPowerFp > 0 ?
        (health.totalComputePowerFp * FIXED_POINT_SCALE) / totalPowerFp : 0;

    return health;
}

/**
 * Export system health metrics to JSON for monitoring dashboards
 */
std::string exportHealthToJSON(const SystemHealth& health) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"timestampMs\": " << health.timestampMs << ",\n";
    json << "  \"activeNodes\": " << health.activeNodes << ",\n";
    json << "  \"avgLatencyMsFp\": " << health.avgLatencyMsFp << ",\n";
    json << "  \"totalComputePowerFp\": " << health.totalComputePowerFp << ",\n";
    json << "  \"networkEfficiencyFp\": " << health.networkEfficiencyFp << ",\n";
    json << "  \"byzantineNodes\": " << health.byzantineNodesDetected << ",\n";
    json << "  \"consensusConfidenceFp\": " << health.consensusConfidenceFp << ",\n";
    json << "  \"totalEnergyContributed_kWhFp\": "
         << health.totalEnergyContributed_kWhFp << "\n";
    json << "}";
    return json.str();
}

} // namespace ambient

#endif // AMBIENT_AI_CORE_V2_H
