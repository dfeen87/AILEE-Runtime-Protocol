// Orchestrator.cpp
// Minimal, compile-safe orchestrator implementations for WeightedOrchestrator.

#include "Orchestrator.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace ailee::sched {

namespace {

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
