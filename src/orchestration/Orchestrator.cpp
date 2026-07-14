// Orchestrator.cpp
// Minimal, compile-safe orchestrator implementations for WeightedOrchestrator.

#include "Orchestrator.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <json/json.h> 

#include "protocol/ProtocolFrame.hpp"
#include "network/MainnetDiscovery.hpp"
#include "BroadcastEngine.hpp" 
#include <openssl/sha.h>

namespace ailee::sched {

namespace {

// ---------------------------------------------------------
// Deterministic signature verification for ProtocolFrame
// ---------------------------------------------------------
static bool verify_activation_frame_signature(const ProtocolFrame& pf)
{
    std::string data = pf.frame_id + pf.type + pf.version +
                       pf.node_id + std::to_string(pf.timestamp) +
                       pf.payload;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), hash);

    std::string hex;
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    static const char* digits = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex.push_back(digits[(hash[i] >> 4) & 0xF]);
        hex.push_back(digits[hash[i] & 0xF]);
    }

    return (hex == pf.signature);
}

// ---------------------------------------------------------
// Activation replay protection + ledger
// ---------------------------------------------------------
struct ActivationLedgerEntry {
    std::string frameId;
    uint64_t    committedAt;
};

class ActivationLedger {
public:
    bool hasSeen(const std::string& frameId) const {
        return seen_.count(frameId) > 0;
    }

    void recordCommit(const std::string& frameId) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        uint64_t ts = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        seen_[frameId] = ActivationLedgerEntry{frameId, ts};
    }

private:
    std::unordered_map<std::string, ActivationLedgerEntry> seen_;
};

// Global ledger instance for now (could be a member later)
static ActivationLedger g_activation_ledger;

// ---------------------------------------------------------
// Basic activation handshake policy
// ---------------------------------------------------------
// For now, we accept activation frames that:
//  - are of type "activation"
//  - have a non-empty node_id
//  - have a valid signature
// Later, this can be extended with posture, reputation, etc.
// ---------------------------------------------------------
static bool activation_handshake_ok(const ProtocolFrame& pf, std::string& reason)
{
    if (pf.type != "activation") {
        reason = "unsupported frame type";
        return false;
    }
    if (pf.node_id.empty()) {
        reason = "missing node_id";
        return false;
    }
    if (!verify_activation_frame_signature(pf)) {
        reason = "invalid activation frame signature";
        return false;
    }
    reason = "ok";
    return true;
}

// ---------------------------------------------------------
// Existing requirement checks (unchanged)
// ---------------------------------------------------------
bool meetsRequirements(const NodeMetrics& node, const ResourceRequirements& req) {
    if (node.capabilities.cpuCores < req.minCpuCores) return false;
    if (node.capabilities.memoryGB < req.minMemoryGB) return false;
    if (node.capabilities.storageGB < req.minStorageGB) return false;
    if (node.bandwidthMbps < req.minBandwidthMbps) return false;
    if (req.requiresGPU && !node.capabilities.hasGPU) return false;
    if (req.requiresTPU && !node.capabilities.hasTPU) return false;
    if (req.minGpuMemoryGB > 0 && node.capabilities.gpuMemoryGB < req.minGpuMemoryGB) return false;
    return true;
}

double latencyScoreFor(const NodeMetrics& node, const ILatencyMap& lat) {
    double latency = node.latencyMs;
    if (auto measured = lat.getLatencyMs(node.peerId)) {
        latency = *measured;
    }
    if (!std::isfinite(latency) || latency <= 0.0) {
        return 0.0;
    }
    return 1.0 / (1.0 + latency);
}

Assignment buildAssignment(const TaskPayload& task, const NodeMetrics& node, double score) {
    Assignment assignment;
    assignment.assigned = true;
    assignment.assignmentId = task.taskId + "-" + node.peerId;
    assignment.assignedAt = std::chrono::system_clock::now();
    assignment.workerPeerId = node.peerId;
    assignment.workerRegion = node.region;
    assignment.finalScore = score;
    assignment.capacityScore = node.capacityScore;
    assignment.expectedLatencyMs = node.latencyMs;
    assignment.costScore = node.costPerHour > 0.0 ? 1.0 / node.costPerHour : 0.0;
    assignment.expectedCostTokens = static_cast<uint64_t>(node.costPerHour);
    return assignment;
}

} // namespace

// ---------------------------------------------------------
// Simple activation commit state (kept inside orchestrator)
// ---------------------------------------------------------
enum class ActivationCommitState : uint8_t {
    INACTIVE = 0,
    HANDSHAKE_OK = 1,
    COMMITTED = 2,
    FAILED = 3
};

// We keep a tiny commit state + last reason inside the orchestrator.
struct ActivationCommitContext {
    ActivationCommitState state = ActivationCommitState::INACTIVE;
    std::string lastReason;
    std::string lastFrameId;
};

Assignment WeightedOrchestrator::assignFromActivationFrame(
    const ProtocolFrame& pf,
    const std::vector<NodeMetrics>& candidates)
{
    ActivationCommitContext ctx;

    // 0. Replay protection: reject already‑committed frame_ids
    if (g_activation_ledger.hasSeen(pf.frame_id)) {
        ctx.state = ActivationCommitState::FAILED;
        ctx.lastReason = "activation replay detected for frame_id=" + pf.frame_id;
        ctx.lastFrameId = pf.frame_id;

        Assignment a;
        a.assigned = false;
        a.reason = ctx.lastReason;
        return a;
    }

    // 1. Handshake
    std::string reason;
    if (!activation_handshake_ok(pf, reason)) {
        ctx.state = ActivationCommitState::FAILED;
        ctx.lastReason = "activation handshake failed: " + reason;
        ctx.lastFrameId = pf.frame_id;

        Assignment a;
        a.assigned = false;
        a.reason = ctx.lastReason;
        return a;
    }

    ctx.state = ActivationCommitState::HANDSHAKE_OK;
    ctx.lastReason = "handshake ok";
    ctx.lastFrameId = pf.frame_id;

    // 2. Minimal mapping from activation frame → TaskPayload
    TaskPayload task;
    task.taskId = pf.frame_id;
    task.requirements = {};

    // 3. Schedule worker
    auto assignment = assignBestWorker(task, candidates, 0.5, 0.3, 0.2);
    if (!assignment.assigned) {
        ctx.state = ActivationCommitState::FAILED;
        ctx.lastReason = "activation commit failed: " + assignment.reason;

        Assignment a;
        a.assigned = false;
        a.reason = ctx.lastReason;
        return a;
    }

    // 4. Commit activation + record in ledger
    ctx.state = ActivationCommitState::COMMITTED;
    ctx.lastReason = "activation committed";
    g_activation_ledger.recordCommit(pf.frame_id);

    return assignment;
}

// ---------------------------------------------------------
// Existing scheduling logic (unchanged)
// ---------------------------------------------------------
Assignment WeightedOrchestrator::assignBestWorker(const TaskPayload& task,
                                                  const std::vector<NodeMetrics>& candidates,
                                                  double trustW,
                                                  double speedW,
                                                  double powerW) const {
    auto filtered = filterCandidates(candidates, task);
    if (filtered.empty()) {
        Assignment assignment;
        assignment.assigned = false;
        assignment.reason = "no viable candidates";
        return assignment;
    }

    double bestScore = -1.0;
    const NodeMetrics* bestNode = nullptr;
    for (const auto& node : filtered) {
        double score = scoreNode(node, task, trustW, speedW, powerW);
        if (score > bestScore) {
            bestScore = score;
            bestNode = &node;
        }
    }

    if (!bestNode) {
        Assignment assignment;
        assignment.assigned = false;
        assignment.reason = "no scoring candidates";
        return assignment;
    }

    metrics_.totalAssignments++;
    metrics_.successfulAssignments++;
    metrics_.assignmentsByWorker[bestNode->peerId]++;
    return buildAssignment(task, *bestNode, bestScore);
}

Assignment WeightedOrchestrator::assignWithStrategy(const TaskPayload& task,
                                                    const std::vector<NodeMetrics>& candidates,
                                                    SchedulingStrategy strategy) const {
    switch (strategy) {
        case SchedulingStrategy::ROUND_ROBIN:
            return assignRoundRobin(candidates);
        case SchedulingStrategy::LEAST_LOADED:
            return assignLeastLoaded(candidates);
        case SchedulingStrategy::LOWEST_LATENCY:
            return assignLowestLatency(candidates);
        case SchedulingStrategy::HIGHEST_REPUTATION:
            return assignHighestReputation(candidates);
        case SchedulingStrategy::LOWEST_COST:
            return assignLowestCost(candidates);
        case SchedulingStrategy::GENETIC_ALGORITHM:
            return assignGeneticAlgorithm(task, candidates);
        case SchedulingStrategy::GEOGRAPHIC_AFFINITY:
        case SchedulingStrategy::LOAD_BALANCING:
            return assignBestWorker(task, candidates, 0.4, 0.4, 0.2);
        case SchedulingStrategy::WEIGHTED_SCORE:
        default:
            return assignBestWorker(task, candidates, 0.5, 0.3, 0.2);
    }
}

std::vector<Assignment> WeightedOrchestrator::assignParallel(
    const std::vector<TaskPayload>& tasks,
    const std::vector<NodeMetrics>& candidates) const {
    std::vector<Assignment> assignments;
    assignments.reserve(tasks.size());
    for (const auto& task : tasks) {
        assignments.push_back(assignWithStrategy(task, candidates, strategy_));
    }
    return assignments;
}

std::vector<std::pair<std::string, double>> WeightedOrchestrator::rankCandidates(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates) const {
    std::vector<std::pair<std::string, double>> ranked;
    for (const auto& node : filterCandidates(candidates, task)) {
        ranked.emplace_back(node.peerId, scoreNode(node, task, 0.5, 0.3, 0.2));
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    return ranked;
}

std::optional<Assignment> WeightedOrchestrator::findBackupWorker(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates,
    const std::string& excludePeerId) const {
    double bestScore = -1.0;
    const NodeMetrics* bestNode = nullptr;
    for (const auto& node : filterCandidates(candidates, task)) {
        if (node.peerId == excludePeerId) continue;
        double score = scoreNode(node, task, 0.4, 0.4, 0.2);
        if (score > bestScore) {
            bestScore = score;
            bestNode = &node;
        }
    }
    if (!bestNode) return std::nullopt;
    return buildAssignment(task, *bestNode, bestScore);
}

std::vector<Assignment> WeightedOrchestrator::scheduleBatch(
    const std::vector<TaskPayload>& tasks,
    const std::vector<NodeMetrics>& candidates) const {
    return assignParallel(tasks, candidates);
}

std::vector<std::pair<std::string, std::string>> WeightedOrchestrator::rebalanceTasks(
    const std::vector<TaskPayload>& tasks,
    const std::vector<NodeMetrics>& candidates) const {
    std::vector<std::pair<std::string, std::string>> rebalanced;
    for (const auto& task : tasks) {
        auto assignment = assignWithStrategy(task, candidates, strategy_);
        if (assignment.assigned) {
            rebalanced.emplace_back(task.taskId, assignment.workerPeerId);
        }
    }
    return rebalanced;
}

std::optional<Assignment> WeightedOrchestrator::findCheapestWorker(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates) const {
    const NodeMetrics* bestNode = nullptr;
    for (const auto& node : filterCandidates(candidates, task)) {
        if (!bestNode || node.costPerHour < bestNode->costPerHour) {
            bestNode = &node;
        }
    }
    if (!bestNode) return std::nullopt;
    return buildAssignment(task, *bestNode, 1.0);
}

uint64_t WeightedOrchestrator::estimateCost(const TaskPayload&, const NodeMetrics& worker) const {
    return static_cast<uint64_t>(worker.costPerHour);
}

Assignment WeightedOrchestrator::optimizeCostPerformance(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates) const {
    double bestScore = -1.0;
    const NodeMetrics* bestNode = nullptr;
    for (const auto& node : filterCandidates(candidates, task)) {
        double costScore = node.costPerHour > 0.0 ? 1.0 / node.costPerHour : 0.0;
        double score = 0.6 * scoreNode(node, task, 0.4, 0.4, 0.2) + 0.4 * costScore;
        if (score > bestScore) {
            bestScore = score;
            bestNode = &node;
        }
    }
    if (!bestNode) {
        Assignment assignment;
        assignment.assigned = false;
        assignment.reason = "no viable candidates";
        return assignment;
    }
    return buildAssignment(task, *bestNode, bestScore);
}

Assignment WeightedOrchestrator::assignRoundRobin(const std::vector<NodeMetrics>& candidates) const {
    if (candidates.empty()) {
        return Assignment{false, "no candidates", "", std::chrono::system_clock::time_point{}, "", "", "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, std::chrono::milliseconds(0), {}};
    }
    return buildAssignment(TaskPayload{}, candidates.front(), 1.0);
}

Assignment WeightedOrchestrator::assignLeastLoaded(const std::vector<NodeMetrics>& candidates) const {
    if (candidates.empty()) {
        return Assignment{false, "no candidates", "", std::chrono::system_clock::time_point{}, "", "", "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, std::chrono::milliseconds(0), {}};
    }
    auto it = std::min_element(candidates.begin(), candidates.end(),
                               [](const NodeMetrics& a, const NodeMetrics& b) {
                                   return a.activeTaskCount < b.activeTaskCount;
                               });
    return buildAssignment(TaskPayload{}, *it, 1.0);
}

Assignment WeightedOrchestrator::assignLowestLatency(const std::vector<NodeMetrics>& candidates) const {
    if (candidates.empty()) {
        return Assignment{false, "no candidates", "", std::chrono::system_clock::time_point{}, "", "", "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, std::chrono::milliseconds(0), {}};
    }
    auto it = std::min_element(candidates.begin(), candidates.end(),
                               [](const NodeMetrics& a, const NodeMetrics& b) {
                                   return a.latencyMs < b.latencyMs;
                               });
    return buildAssignment(TaskPayload{}, *it, 1.0);
}

Assignment WeightedOrchestrator::assignHighestReputation(const std::vector<NodeMetrics>& candidates) const {
    if (candidates.empty()) {
        return Assignment{false, "no candidates", "", std::chrono::system_clock::time_point{}, "", "", "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, std::chrono::milliseconds(0), {}};
    }
    double bestScore = -1.0;
    const NodeMetrics* bestNode = nullptr;
    for (const auto& node : candidates) {
        double score = rep_.get(node.peerId).score();
        if (score > bestScore) {
            bestScore = score;
            bestNode = &node;
        }
    }
    if (!bestNode) {
        return Assignment{false, "no candidates", "", std::chrono::system_clock::time_point{}, "", "", "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, std::chrono::milliseconds(0), {}};
    }
    return buildAssignment(TaskPayload{}, *bestNode, bestScore);
}

Assignment WeightedOrchestrator::assignLowestCost(const std::vector<NodeMetrics>& candidates) const {
    if (candidates.empty()) {
        return Assignment{false, "no candidates", "", std::chrono::system_clock::time_point{}, "", "", "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, std::chrono::milliseconds(0), {}};
    }
    auto it = std::min_element(candidates.begin(), candidates.end(),
                               [](const NodeMetrics& a, const NodeMetrics& b) {
                                   return a.costPerHour < b.costPerHour;
                               });
    return buildAssignment(TaskPayload{}, *it, 1.0);
}

Assignment WeightedOrchestrator::assignGeneticAlgorithm(const TaskPayload& task,
                                                        const std::vector<NodeMetrics>& candidates) const {
    return assignBestWorker(task, candidates, 0.5, 0.3, 0.2);
}

double WeightedOrchestrator::scoreNode(const NodeMetrics& node,
                                       const TaskPayload& task,
                                       double trustW,
                                       double speedW,
                                       double powerW) const {
    double trustScore = rep_.get(node.peerId).score();
    double latencyScore = latencyScoreFor(node, lat_);
    double capacityScore = node.capacityScore;
    if (task.preferGreenEnergy) {
        capacityScore += (node.carbonIntensity > 0.0) ? (1.0 / (1.0 + node.carbonIntensity)) : 0.0;
    }
    return trustW * trustScore + speedW * latencyScore + powerW * capacityScore;
}

// ---------------------------------------------------------
// Mainnet Activation Projection Hook
// ---------------------------------------------------------
// After activation is committed locally, we project the activation
// to all verified mainnet peers using BroadcastEngine.
// This integrates activation into the mainnet propagation pipeline.
// ---------------------------------------------------------
Assignment WeightedOrchestrator::projectActivationToMainnet(
    const ProtocolFrame& pf,
    const std::vector<NodeMetrics>& candidates,
    MainnetDiscovery* discovery) const
{
    // Step 1: Perform handshake + commit using existing logic
    auto assignment = assignFromActivationFrame(pf, candidates);
    if (!assignment.assigned) {
        // If activation fails locally, do NOT project it
        return assignment;
    }

    // Step 2: Only project if discovery subsystem is available
    if (!discovery) {
        return assignment;
    }

    // Step 3: Build minimal projection payload
    Json::Value payload;
    payload["frame_id"] = pf.frame_id;
    payload["node_id"]  = pf.node_id;
    payload["type"]     = pf.type;
    payload["version"]  = pf.version;

    // Step 4: Emit projection to all verified mainnet peers
    BroadcastEngine::emit("activation_projection", pf.version, payload);

    return assignment;
}

std::vector<NodeMetrics> WeightedOrchestrator::filterCandidates(
    const std::vector<NodeMetrics>& candidates,
    const TaskPayload& task) const {
    std::vector<NodeMetrics> filtered;
    for (const auto& node : candidates) {
        if (!meetsRequirements(node, task.requirements)) {
            continue;
        }
        if (task.preferredRegion && node.region != *task.preferredRegion) {
            continue;
        }
        if (!task.blacklistedNodes.empty() &&
            std::find(task.blacklistedNodes.begin(), task.blacklistedNodes.end(), node.peerId) !=
                task.blacklistedNodes.end()) {
            continue;
        }
        if (task.maxCostTokens > 0 && node.costPerHour > task.maxCostTokens) {
            continue;
        }
        filtered.push_back(node);
    }
    return filtered;
}

} // namespace ailee::sched
