// SPDX-License-Identifier: MIT
// AmbientAI.cpp — Production-grade implementation for Decentralized Ambient AI Infrastructure
// Fully integrated with AILEE-Core AmbientNode, ZK proofs, federated learning, token incentives,
// Byzantine fault detection, and system health metrics.

#include "AmbientAI.h"
#include "crypto_utils.h"
#include "zk_proofs.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <openssl/sha.h>

namespace ambient {

namespace {

// Delegate to the shared utility in crypto_utils.h to avoid duplicate implementations.
inline std::string sha256Hex(const std::string& input) {
    return ailee::crypto::sha256_hex(input);
}

inline std::string sha256Hex(const std::vector<uint8_t>& input) {
    return ailee::crypto::sha256_hex(input);
}

} // namespace

// ============================================================================
// SERIALIZATION & STRING CONVERSION
// ============================================================================

std::string to_string(const NodeId& n) {
    std::ostringstream ss;
    ss << "NodeId(pubkey=" << n.pubkey.substr(0,16) << "..."
       << ", region=" << n.region
       << ", deviceClass=" << n.deviceClass << ")";
    return ss.str();
}

std::string to_string(const EnergyProfile& e) {
    std::ostringstream ss;
    ss << "Energy(input=" << std::fixed << std::setprecision(2) << e.inputPowerW << "W"
       << ", recovered=" << e.wasteHeatRecoveredW << "W"
       << ", temp=" << e.temperatureC << "°C"
       << ", ambient=" << e.ambientTempC << "°C"
       << ", carbon=" << e.carbonIntensity_gCO2_kWh << "gCO2/kWh"
       << ", efficiency=" << e.computeEfficiency_GFLOPS_W << " GFLOPS/W)";
    return ss.str();
}

std::string to_string(const ComputeProfile& c) {
    std::ostringstream ss;
    ss << "Compute(cpu=" << std::fixed << std::setprecision(1) << c.cpuUtilization << "%"
       << ", npu=" << c.npuUtilization << "%"
       << ", gpu=" << c.gpuUtilization << "%"
       << ", mem=" << c.availableMemMB << "MB"
       << ", bw=" << c.bandwidthMbps << "Mbps"
       << ", lat=" << std::setprecision(2) << c.latencyMs << "ms"
       << ", power=" << c.instantaneousPower_GFLOPS << " GFLOPS)";
    return ss.str();
}

std::string to_string(const PrivacyBudget& p) {
    std::ostringstream ss;
    ss << "Privacy(ε=" << std::scientific << std::setprecision(2) << p.epsilon
       << ", δ=" << p.delta
       << ", remaining=" << p.privacyBudgetRemaining
       << ", HE=" << p.homomorphicEncryptionEnabled
       << ", ZKP=" << p.zeroKnowledgeProofEnabled
       << ")";
    return ss.str();
}

std::string to_string(const TelemetrySample& s) {
    std::ostringstream ss;
    ss << "Telemetry@t=" << std::chrono::duration_cast<std::chrono::milliseconds>(
        s.timestamp.time_since_epoch()).count() << "ms {\n"
       << "  " << to_string(s.node) << "\n"
       << "  " << to_string(s.energy) << "\n"
       << "  " << to_string(s.compute) << "\n"
       << "  " << to_string(s.privacy) << "\n";
    return ss.str();
}

// ============================================================================
// CRYPTOGRAPHIC VERIFICATION (ZK Proof Integration)
// ============================================================================

std::string computeVerificationHash(const TelemetrySample& sample) {
    ailee::zk::ZKEngine zkEngine;
    auto proof = zkEngine.generateProof(sample.node.pubkey,
                                        sha256Hex(std::to_string(sample.timestamp.time_since_epoch().count()) +
                                                  std::to_string(sample.energy.inputPowerW)));
    return proof.proofData;
}

bool verifyComputationProof(const TelemetrySample& sample) {
    if (sample.privacy.zeroKnowledgeProofEnabled == false) return true;
    ailee::zk::ZKEngine zkEngine;
    if (sample.cryptographicVerificationHash.empty()) return false;
    const std::string inputHash = sha256Hex(std::to_string(sample.timestamp.time_since_epoch().count()) +
                                            std::to_string(sample.energy.inputPowerW));
    ailee::zk::Proof proof{sample.cryptographicVerificationHash,
                           sample.node.pubkey + ":" + inputHash,
                           std::nullopt,
                           false,
                           {}};
    return zkEngine.verifyProof(proof);
}

FederatedUpdate AmbientNode::runLocalTraining(const ailee::fl::FLTask& task,
                                              const std::vector<uint8_t>& deltaBytes,
                                              std::size_t numSamplesTrained,
                                              double trainingLoss,
                                              std::chrono::milliseconds computeTime) {
    std::lock_guard<std::mutex> lock(mu_);
    FederatedUpdate update;
    update.workerId = id_.pubkey;
    update.taskId = task.taskId;
    update.roundNumber = task.currentRound;
    update.modelHash = task.globalModelHash;
    update.deltaBytes = deltaBytes;
    update.compression = ailee::fl::CompressionMethod::NONE;
    update.numSamplesTrained = numSamplesTrained;
    update.numEpochs = task.localEpochs;
    update.trainingLoss = trainingLoss;
    if (lastSample_.has_value()) {
        update.epsilonSpent = lastSample_->privacy.epsilon;
        update.deltaSpent = lastSample_->privacy.delta;
        update.isDPNoisyUpdate = lastSample_->privacy.epsilon < 1.0;
    }
    update.submissionTime = std::chrono::system_clock::now();
    update.computeTime = computeTime;

    const std::string deltaHash = sha256Hex(deltaBytes);
    ailee::zk::ZKEngine zkEngine;
    auto proof = zkEngine.generateProof(task.taskId, deltaHash);
    update.proofBytes.assign(proof.proofData.begin(), proof.proofData.end());
    update.proofHash = proof.proofData;
    update.proofVerified = zkEngine.verifyProof(proof);

    return update;
}

// ============================================================================
// NASH EQUILIBRIUM & GAME THEORY CALCULATIONS
// ============================================================================

double calculateNodeUtility(const AmbientNode& node, double tokenRewardRate) {
    auto sampleOpt = node.last();
    if (!sampleOpt.has_value()) return 0.0;
    const auto& s = sampleOpt.value();
    double computeContribution = s.compute.instantaneousPower_GFLOPS;
    double energyCost = s.energy.inputPowerW * 0.001;
    double latencyPenalty = s.compute.latencyMs * 0.01;
    double baseReward = computeContribution * tokenRewardRate;
    return std::max(0.0, baseReward - energyCost - latencyPenalty);
}

double calculateNashEquilibriumThreshold(const std::vector<AmbientNode*>& network) {
    if (network.empty()) return 0.0;
    double totalCompute = 0.0, totalEnergy = 0.0;
    for (auto* n : network) {
        auto sampleOpt = n->last();
        if (!sampleOpt.has_value()) continue;
        const auto& s = sampleOpt.value();
        totalCompute += s.compute.instantaneousPower_GFLOPS;
        totalEnergy += s.energy.inputPowerW;
    }
    return totalEnergy > 0 ? totalCompute / totalEnergy : 0.0;
}

// ============================================================================
// BYZANTINE FAULT DETECTION
// ============================================================================

bool detectByzantineNode(const AmbientNode& node, const std::vector<AmbientNode*>& peers, double threshold) {
    auto sampleOpt = node.last();
    if (!sampleOpt.has_value() || peers.size()<3) return false;
    std::vector<double> vals;
    for (auto* p: peers) {
        auto sOpt = p->last();
        if (!sOpt.has_value()) continue;
        vals.push_back(sOpt->compute.instantaneousPower_GFLOPS);
    }
    if(vals.size()<3) return false;
    std::sort(vals.begin(), vals.end());
    double median = vals[vals.size()/2];
    double mad = 0.0;
    for (auto v: vals) mad += std::abs(v-median);
    mad /= vals.size();
    double modZ = 0.6745 * std::abs(sampleOpt->compute.instantaneousPower_GFLOPS - median)/(mad+1e-9);
    if(modZ>threshold) std::cout << "[BYZANTINE DETECTED] Node " << node.id().pubkey << "\n";
    return modZ>threshold;
}

// ============================================================================
// REPUTATION & TOKEN ECONOMICS
// ============================================================================

double updateReputationScore(const AmbientNode& node, bool success, double sla, int64_t uptimeMs) {
    auto rep = node.reputation();
    double delta = success ? 0.01*sla : -0.05;
    delta += std::min(0.01, uptimeMs/3600000.0*0.001);
    double newScore = 0.95*rep.score + 0.05*(rep.score+delta);
    return std::clamp(newScore,0.0,1.0);
}

IncentiveRecord calculateTokenReward(const AmbientNode& node, double baseRate) {
    auto sampleOpt = node.last();
    if(!sampleOpt.has_value()) return {};
    const auto& s = sampleOpt.value();
    double eff = s.energy.computeEfficiency_GFLOPS_W;
    double priv = s.privacy.privacyBudgetRemaining;
    double rep = node.reputation().score;
    double amount = s.compute.instantaneousPower_GFLOPS*baseRate*eff*priv*rep;
    return node.accrueReward("autoTask",amount);
}

// ============================================================================
// SYSTEM HEALTH & VALIDATION
// ============================================================================

struct SystemHealth {
    size_t activeNodes = 0;
    double avgLatency_ms = 0.0;
    double totalComputePower_GFLOPS = 0.0;
    double networkEfficiency = 0.0;
    double aggregatePrivacyBudget = 0.0;
    size_t byzantineNodesDetected = 0;
};

SystemHealth analyzeSystemHealth(const std::vector<AmbientNode*>& network) {
    SystemHealth health;
    if(network.empty()) return health;
    health.activeNodes = network.size();
    for(auto* n: network){
        auto sOpt = n->last();
        if(!sOpt.has_value()) continue;
        const auto& s = sOpt.value();
        health.avgLatency_ms += s.compute.latencyMs;
        health.totalComputePower_GFLOPS += s.compute.instantaneousPower_GFLOPS;
        health.aggregatePrivacyBudget += s.privacy.epsilon;
        if(!verifyComputationProof(s)) health.byzantineNodesDetected++;
    }
    health.avgLatency_ms /= health.activeNodes;
    health.aggregatePrivacyBudget /= health.activeNodes;
    double totalPower=0;
    for(auto* n: network){
        auto sOpt = n->last();
        if(!sOpt.has_value()) continue;
        totalPower += sOpt->energy.inputPowerW;
    }
    health.networkEfficiency = totalPower>0 ? health.totalComputePower_GFLOPS/totalPower : 0;
    return health;
}

void validateTelemetrySample(const TelemetrySample& s){
    if(s.node.pubkey.empty()) throw std::invalid_argument("Node public key cannot be empty");
    if(s.compute.latencyMs<0) throw std::invalid_argument("Latency cannot be negative");
    if(s.energy.inputPowerW<0) throw std::invalid_argument("Power cannot be negative");
    if(s.privacy.epsilon>10.0) throw std::invalid_argument("Privacy epsilon exceeds threshold");
    if(s.privacy.epsilon<0) throw std::invalid_argument("Privacy budget exhausted");
    if(!verifyComputationProof(s)) throw std::invalid_argument("ZK proof verification failed");
}

} // namespace ambient
