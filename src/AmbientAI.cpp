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
    ss << "Energy(input=" << e.inputPowerWFp << "W (scaled)"
       << ", recovered=" << e.wasteHeatRecoveredWFp << "W (scaled)"
       << ", temp=" << e.temperatureCFp << "°C (scaled)"
       << ", ambient=" << e.ambientTempCFp << "°C (scaled)"
       << ", carbon=" << e.carbonIntensity_gCO2_kWhFp << "gCO2/kWh (scaled)"
       << ", efficiency=" << e.computeEfficiency_GFLOPS_WFp << " GFLOPS/W (scaled))";
    return ss.str();
}

std::string to_string(const ComputeProfile& c) {
    std::ostringstream ss;
    ss << "Compute(cpu=" << c.cpuUtilizationFp << "% (scaled)"
       << ", npu=" << c.npuUtilizationFp << "% (scaled)"
       << ", gpu=" << c.gpuUtilizationFp << "% (scaled)"
       << ", mem=" << c.availableMemMBFp << "MB (scaled)"
       << ", bw=" << c.bandwidthMbpsFp << "Mbps (scaled)"
       << ", lat=" << c.latencyMsFp << "ms (scaled)"
       << ", power=" << c.instantaneousPower_GFLOPSFp << " GFLOPS (scaled))";
    return ss.str();
}

std::string to_string(const PrivacyBudget& p) {
    std::ostringstream ss;
    ss << "Privacy(ε=" << p.epsilonFp << " (scaled)"
       << ", δ=" << p.deltaFp << " (scaled)"
       << ", remaining=" << p.privacyBudgetRemainingFp << " (scaled)"
       << ", HE=" << p.homomorphicEncryptionEnabled
       << ", ZKP=" << p.zeroKnowledgeProofEnabled
       << ")";
    return ss.str();
}

std::string to_string(const TelemetrySample& s) {
    std::ostringstream ss;
    ss << "Telemetry@t=" << s.protocolTimestampMs << "ms {\n"
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
                                        sha256Hex(std::to_string(sample.protocolTimestampMs) +
                                                  std::to_string(sample.energy.inputPowerWFp)));
    return proof.proofData;
}

bool verifyComputationProof(const TelemetrySample& sample) {
    if (sample.privacy.zeroKnowledgeProofEnabled == false) return true;
    ailee::zk::ZKEngine zkEngine;
    if (sample.cryptographicVerificationHash.empty()) return false;
    const std::string inputHash = sha256Hex(std::to_string(sample.protocolTimestampMs) +
                                            std::to_string(sample.energy.inputPowerWFp));
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
                                              uint64_t trainingLossFp,
                                              uint64_t computeTimeMs,
                                              uint64_t protocolTimestampMs) {
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
    update.trainingLoss = static_cast<double>(trainingLossFp) / FIXED_POINT_SCALE; // Update FederatedUpdate or do this conversion for compatibility
    if (lastSample_.has_value()) {
        update.epsilonSpent = static_cast<double>(lastSample_->privacy.epsilonFp) / FIXED_POINT_SCALE;
        update.deltaSpent = static_cast<double>(lastSample_->privacy.deltaFp) / FIXED_POINT_SCALE;
        update.isDPNoisyUpdate = lastSample_->privacy.epsilonFp < FIXED_POINT_SCALE;
    }
    update.submissionTime = std::chrono::system_clock::time_point(std::chrono::milliseconds(protocolTimestampMs));
    update.computeTime = std::chrono::milliseconds(computeTimeMs);

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

int64_t calculateNodeUtilityFp(const AmbientNode& node, uint64_t tokenRewardRateFp) {
    auto sampleOpt = node.last();
    if (!sampleOpt.has_value()) return 0;
    const auto& s = sampleOpt.value();
    uint64_t computeContributionFp = s.compute.instantaneousPower_GFLOPSFp;

    // energyCost = (inputPower * 0.001 scaled)
    uint64_t energyCostFp = (s.energy.inputPowerWFp * 10) / FIXED_POINT_SCALE; // 0.001 is 10/10000
    // latencyPenalty = (latency * 0.01 scaled)
    uint64_t latencyPenaltyFp = (s.compute.latencyMsFp * 100) / FIXED_POINT_SCALE; // 0.01 is 100/10000

    uint64_t baseRewardFp = (computeContributionFp * tokenRewardRateFp) / FIXED_POINT_SCALE;
    int64_t utility = static_cast<int64_t>(baseRewardFp) - static_cast<int64_t>(energyCostFp) - static_cast<int64_t>(latencyPenaltyFp);
    return std::max<int64_t>(0, utility);
}

uint64_t calculateNashEquilibriumThresholdFp(const std::vector<AmbientNode*>& network) {
    if (network.empty()) return 0;
    uint64_t totalComputeFp = 0, totalEnergyFp = 0;
    for (auto* n : network) {
        auto sampleOpt = n->last();
        if (!sampleOpt.has_value()) continue;
        const auto& s = sampleOpt.value();
        totalComputeFp += s.compute.instantaneousPower_GFLOPSFp;
        totalEnergyFp += s.energy.inputPowerWFp;
    }
    return totalEnergyFp > 0 ? (totalComputeFp * FIXED_POINT_SCALE) / totalEnergyFp : 0;
}

// ============================================================================
// BYZANTINE FAULT DETECTION
// ============================================================================

bool detectByzantineNode(const AmbientNode& node, const std::vector<AmbientNode*>& peers, uint64_t thresholdFp) {
    auto sampleOpt = node.last();
    if (!sampleOpt.has_value() || peers.size()<3) return false;
    std::vector<uint64_t> vals;
    for (auto* p: peers) {
        auto sOpt = p->last();
        if (!sOpt.has_value()) continue;
        vals.push_back(sOpt->compute.instantaneousPower_GFLOPSFp);
    }
    if(vals.size()<3) return false;
    std::sort(vals.begin(), vals.end());
    uint64_t medianFp = vals[vals.size()/2];

    uint64_t madFp = 0;
    for (auto v: vals) {
        madFp += (v > medianFp) ? (v - medianFp) : (medianFp - v);
    }
    madFp /= vals.size();

    uint64_t valFp = sampleOpt->compute.instantaneousPower_GFLOPSFp;
    uint64_t diffFp = (valFp > medianFp) ? (valFp - medianFp) : (medianFp - valFp);

    uint64_t denominatorFp = madFp + 1; // +1e-9 avoiding division by zero
    // 0.6745 is 6745/10000
    uint64_t modZFp = (6745 * diffFp) / denominatorFp;

    if(modZFp > thresholdFp) std::cout << "[BYZANTINE DETECTED] Node " << node.id().pubkey << "\n";
    return modZFp > thresholdFp;
}

// ============================================================================
// REPUTATION & TOKEN ECONOMICS
// ============================================================================

uint64_t updateReputationScoreFp(const AmbientNode& node, bool success, uint64_t slaFp, int64_t uptimeMs) {
    auto rep = node.reputation();
    int64_t deltaFp = 0;
    if (success) {
        deltaFp = (100 * slaFp) / FIXED_POINT_SCALE; // 0.01 * sla
    } else {
        deltaFp = -500; // -0.05
    }

    // std::min(0.01, uptimeMs/3600000.0*0.001)
    uint64_t uptimeBonusFp = (uptimeMs * 10) / 3600000;
    if (uptimeBonusFp > 100) uptimeBonusFp = 100;

    deltaFp += uptimeBonusFp;

    int64_t currentScoreFp = rep.scoreFp;
    int64_t term1 = (9500 * currentScoreFp) / FIXED_POINT_SCALE; // 0.95
    int64_t term2 = (500 * (currentScoreFp + deltaFp)) / FIXED_POINT_SCALE; // 0.05

    int64_t newScoreFp = term1 + term2;
    if (newScoreFp < 0) newScoreFp = 0;
    if (newScoreFp > static_cast<int64_t>(FIXED_POINT_SCALE)) newScoreFp = FIXED_POINT_SCALE;

    return static_cast<uint64_t>(newScoreFp);
}

IncentiveRecord calculateTokenReward(const AmbientNode& node, uint64_t baseRateFp) {
    auto sampleOpt = node.last();
    if(!sampleOpt.has_value()) return {};
    const auto& s = sampleOpt.value();
    uint64_t effFp = s.energy.computeEfficiency_GFLOPS_WFp;
    uint64_t privFp = s.privacy.privacyBudgetRemainingFp;
    uint64_t repFp = node.reputation().scoreFp;

    uint64_t amount1 = (s.compute.instantaneousPower_GFLOPSFp * baseRateFp) / FIXED_POINT_SCALE;
    uint64_t amount2 = (amount1 * effFp) / FIXED_POINT_SCALE;
    uint64_t amount3 = (amount2 * privFp) / FIXED_POINT_SCALE;
    uint64_t finalAmountFp = (amount3 * repFp) / FIXED_POINT_SCALE;

    return node.accrueReward("autoTask", finalAmountFp);
}

// ============================================================================
// SYSTEM HEALTH & VALIDATION
// ============================================================================

struct SystemHealth {
    size_t activeNodes = 0;
    uint64_t avgLatencyMsFp = 0;
    uint64_t totalComputePower_GFLOPSFp = 0;
    uint64_t networkEfficiencyFp = 0;
    uint64_t aggregatePrivacyBudgetFp = 0;
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
        health.avgLatencyMsFp += s.compute.latencyMsFp;
        health.totalComputePower_GFLOPSFp += s.compute.instantaneousPower_GFLOPSFp;
        health.aggregatePrivacyBudgetFp += s.privacy.epsilonFp;
        if(!verifyComputationProof(s)) health.byzantineNodesDetected++;
    }
    health.avgLatencyMsFp /= health.activeNodes;
    health.aggregatePrivacyBudgetFp /= health.activeNodes;
    uint64_t totalPowerFp = 0;
    for(auto* n: network){
        auto sOpt = n->last();
        if(!sOpt.has_value()) continue;
        totalPowerFp += sOpt->energy.inputPowerWFp;
    }
    health.networkEfficiencyFp = totalPowerFp > 0 ? (health.totalComputePower_GFLOPSFp * FIXED_POINT_SCALE) / totalPowerFp : 0;
    return health;
}

void validateTelemetrySample(const TelemetrySample& s){
    if(s.node.pubkey.empty()) throw std::invalid_argument("Node public key cannot be empty");
    // Removed latency and input power negative checks because they are now unsigned uint64_t
    if(s.privacy.epsilonFp > 10 * FIXED_POINT_SCALE) throw std::invalid_argument("Privacy epsilon exceeds threshold");
    // Removed epsilon negative check because it is now unsigned uint64_t
    if(!verifyComputationProof(s)) throw std::invalid_argument("ZK proof verification failed");
}

} // namespace ambient
