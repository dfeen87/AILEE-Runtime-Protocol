// SPDX-License-Identifier: MIT
// Engine.h — Unified Orchestration Engine for AILEE-Core
// Integrates reputation, latency tracking, node discovery, and task orchestration
// into a cohesive runtime system.

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <future>
#include "ProverSwarm.h"

namespace ailee::sched {

// ==================== CORE TYPES ====================

enum class SchedulingStrategy {
    WEIGHTED_SCORE,
    ROUND_ROBIN,
    LEAST_LOADED,
    LOWEST_LATENCY,
    HIGHEST_REPUTATION,
    LOWEST_COST,
    GENETIC_ALGORITHM,
    GEOGRAPHIC_AFFINITY,
    LOAD_BALANCING
};

enum class TaskType {
    AI_INFERENCE,
    AI_TRAINING,
    FEDERATED_LEARNING,
    WASM_EXECUTION,
    ZK_PROOF_GENERATION,
    DATA_PROCESSING,
    BANDWIDTH_RELAY
};

enum class TaskPriority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

struct ResourceRequirements {
    uint32_t minCpuCores = 1;
    uint32_t minMemoryGB = 1;
    uint32_t minStorageGB = 1;
    double minBandwidthMbps = 1.0;
    bool requiresGPU = false;
    uint32_t minGpuMemoryGB = 0;
    bool requiresTPU = false;
    std::vector<std::string> requiredCapabilities;
};

struct NodeCapabilities {
    uint32_t cpuCores = 1;
    uint32_t memoryGB = 1;
    uint32_t storageGB = 0;
    uint32_t gpuMemoryGB = 0;
    bool hasGPU = false;
    bool hasTPU = false;
    std::vector<std::string> supportedArchitectures;
    std::vector<std::string> runtimeVersions;
};

struct NodeMetrics {
    std::string peerId;
    std::string region;
    double bandwidthMbps = 0.0;
    double latencyMs = std::numeric_limits<double>::infinity();
    double cpuUtilization = 0.0;
    double capacityScore = 0.0;
    double costPerHour = 0.0;
    uint64_t tokensAvailable = 0;
    uint32_t maxConcurrentTasks = 0;
    uint32_t activeTaskCount = 0;
    double carbonIntensity = 0.0;
    std::chrono::system_clock::time_point lastSeen{};
    NodeCapabilities capabilities;
};

struct TaskPayload {
    std::string taskId;
    TaskType taskType = TaskType::DATA_PROCESSING;
    TaskPriority priority = TaskPriority::NORMAL;
    std::string submitterId;
    std::chrono::system_clock::time_point submittedAt{};
    ResourceRequirements requirements;
    uint64_t maxCostTokens = 0;
    double minReputationScore = 0.0;
    std::optional<std::string> preferredRegion;
    std::optional<std::string> anchorCommitmentHash; // Optional L2 anchor reference
    bool preferGreenEnergy = false;
    std::vector<std::string> blacklistedNodes;
    std::vector<std::uint8_t> payloadBytes;
};

struct Assignment {
    bool assigned = false;
    std::string reason;
    std::string assignmentId;
    std::chrono::system_clock::time_point assignedAt{};
    std::string workerPeerId;
    std::string workerRegion;
    std::string backupWorkerPeerId;
    double finalScore = 0.0;
    double reputationScore = 0.0;
    double expectedLatencyMs = std::numeric_limits<double>::infinity();
    double latencyScore = 0.0;
    double capacityScore = 0.0;
    double costScore = 0.0;
    uint64_t expectedCostTokens = 0;
    std::chrono::milliseconds estimatedCompletionTime{0};
    std::vector<std::pair<std::string, double>> candidateScores;
};

struct Reputation {
    std::string peerId;
    double trustScore = 0.5;
    double allTimeSuccessRate = 0.0;
    uint64_t totalTasks = 0;
    uint64_t successfulTasks = 0;
    uint64_t failedTasks = 0;
    double avgQualityScore = 0.0;
    double avgResponseTime = 0.0;
    uint64_t byzantineBehaviors = 0;
    uint64_t totalSlashings = 0;
    std::chrono::system_clock::time_point lastUpdated{};

    double score() const {
        double base = trustScore;
        if (totalTasks > 0) {
            double responseFactor = 1.0 - std::min(avgResponseTime / 10.0, 1.0);
            base = (allTimeSuccessRate * 0.7) + (avgQualityScore * 0.2) + (responseFactor * 0.1);
        }
        return std::clamp(base, 0.0, 1.0);
    }

    void decay(std::chrono::seconds elapsed) {
        double decayFactor = std::min(0.5, elapsed.count() / 86400.0 * 0.01);
        trustScore = std::max(0.0, trustScore - decayFactor);
    }
};

class IReputationLedger {
public:
    virtual ~IReputationLedger() = default;
    virtual Reputation get(const std::string& peerId) const = 0;
    virtual void update(const std::string& peerId, int deltaSuccess, int deltaFailure) = 0;
    virtual void updateBatch(const std::vector<std::pair<std::string, std::pair<int, int>>>& updates) = 0;
    virtual void recordTaskCompletion(const std::string& peerId, bool success,
                                      double qualityScore, std::chrono::milliseconds responseTime) = 0;
    virtual void recordByzantineBehavior(const std::string& peerId, const std::string& reason) = 0;
    virtual void rewardNode(const std::string& peerId, double reputationBoost) = 0;
    virtual void slashNode(const std::string& peerId, double reputationPenalty, uint64_t tokenSlash) = 0;
    virtual std::vector<std::string> getTopNodes(std::size_t n) const = 0;
    virtual std::vector<std::string> getNodesAboveThreshold(double threshold) const = 0;
    virtual std::unordered_map<std::string, Reputation> getAllReputations() const = 0;
    virtual void decayInactiveNodes(std::chrono::seconds inactivityThreshold) = 0;
    virtual bool resetReputation(const std::string& peerId, const std::string& reason) = 0;
    virtual std::vector<std::string> exportReputationLog() const = 0;
};

class ILatencyMap {
public:
    virtual ~ILatencyMap() = default;
    virtual std::optional<double> getLatencyMs(const std::string& peerId) const = 0;
    virtual void updateLatency(const std::string& peerId, double latencyMs) = 0;
    virtual std::optional<double> getBandwidthMbps(const std::string& peerId) const = 0;
    virtual std::optional<double> getJitterMs(const std::string& peerId) const = 0;
    virtual std::optional<double> probeLatency(const std::string& peerId) = 0;
    virtual std::optional<double> getDistanceKm(const std::string& peerId) const = 0;
    virtual std::unordered_map<std::string, double> getAllLatencies() const = 0;
    virtual void cleanupStale(std::chrono::seconds maxAge) = 0;
};

class IOrchestrator {
public:
    struct OrchestratorMetrics {
        uint64_t totalAssignments = 0;
        uint64_t successfulAssignments = 0;
        uint64_t failedAssignments = 0;
        std::chrono::milliseconds avgAssignmentTime{0};
        std::unordered_map<std::string, uint64_t> assignmentsByWorker;
    };

    virtual ~IOrchestrator() = default;
    virtual Assignment assignBestWorker(const TaskPayload& task,
                                        const std::vector<NodeMetrics>& candidates,
                                        double trustW,
                                        double speedW,
                                        double powerW) const = 0;
    virtual Assignment assignWithStrategy(const TaskPayload& task,
                                          const std::vector<NodeMetrics>& candidates,
                                          SchedulingStrategy strategy) const = 0;
    virtual void setStrategy(SchedulingStrategy strategy) = 0;
    virtual SchedulingStrategy getStrategy() const = 0;
    virtual OrchestratorMetrics getMetrics() const = 0;
};

// ==================== CONFIGURATION ====================

struct NetworkConfig {
    std::string listenAddress = "0.0.0.0";
    uint16_t listenPort = 8080;
    std::size_t maxConnections = 1000;
    std::chrono::seconds connectionTimeout{30};
    std::chrono::seconds heartbeatInterval{10};
    bool enableTLS = true;
    std::string tlsCertPath;
    std::string tlsKeyPath;
};

struct PerformanceConfig {
    SchedulingStrategy defaultStrategy = SchedulingStrategy::WEIGHTED_SCORE;
    uint32_t maxConcurrentTasks = 100;
    uint32_t workerThreads = 4;
    std::chrono::milliseconds taskTimeout{30000};
    double trustWeight = 0.6;
    double speedWeight = 0.3;
    double powerWeight = 0.1;
    bool enableAdaptiveScheduling = true;
};

struct EconomicConfig {
    uint64_t defaultMaxCostTokens = 1000;
    double minReputationThreshold = 0.5;
    bool enableDynamicPricing = true;
    double priceAdjustmentRate = 0.1;
    uint64_t slashingPenalty = 100;
    double reputationDecayRate = 0.01;
};

struct MonitoringConfig {
    bool enableMetrics = true;
    bool enableLogging = true;
    std::string metricsEndpoint = "localhost:9090";
    std::chrono::seconds metricsInterval{60};
    std::string logLevel = "INFO";
    std::string logPath = "./logs/engine.log";
};

struct Config {
    NetworkConfig network;
    PerformanceConfig performance;
    EconomicConfig economic;
    MonitoringConfig monitoring;
    
    // Feature flags
    bool enableZKProofs = true;
    bool enableFederatedLearning = true;
    bool enableGreenScheduling = false;
    bool enableLoadRebalancing = true;
    
    // Discovery settings
    std::vector<std::string> bootstrapPeers;
    std::chrono::seconds discoveryInterval{30};
    std::size_t minPeerCount = 3;
};

// ==================== CONCRETE IMPLEMENTATIONS ====================

class ReputationLedger final : public IReputationLedger {
public:
    ReputationLedger() = default;
    
    Reputation get(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = reputations_.find(peerId);
        if (it != reputations_.end()) {
            return it->second;
        }
        // Return new neutral reputation
        Reputation rep;
        rep.peerId = peerId;
        rep.trustScore = 0.5;
        return rep;
    }
    
    void update(const std::string& peerId, int deltaSuccess, int deltaFailure) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.peerId = peerId;
        rep.totalTasks += deltaSuccess + deltaFailure;
        rep.successfulTasks += deltaSuccess;
        rep.failedTasks += deltaFailure;
        rep.lastUpdated = std::chrono::system_clock::now();
        
        // Update all-time success rate
        if (rep.totalTasks > 0) {
            rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / rep.totalTasks;
        }
        
        // Update trust score
        rep.trustScore = rep.score();
    }
    
    void updateBatch(const std::vector<std::pair<std::string, std::pair<int, int>>>& updates) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [peerId, deltas] : updates) {
            auto& rep = reputations_[peerId];
            rep.peerId = peerId;
            rep.totalTasks += deltas.first + deltas.second;
            rep.successfulTasks += deltas.first;
            rep.failedTasks += deltas.second;
            rep.lastUpdated = std::chrono::system_clock::now();
            
            if (rep.totalTasks > 0) {
                rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / rep.totalTasks;
            }
            rep.trustScore = rep.score();
        }
    }
    
    void recordTaskCompletion(const std::string& peerId, bool success, 
                            double qualityScore, std::chrono::milliseconds responseTime) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.peerId = peerId;
        
        if (success) {
            rep.successfulTasks++;
        } else {
            rep.failedTasks++;
        }
        rep.totalTasks++;
        
        // Update quality metrics
        double alpha = 0.1; // Exponential moving average factor
        rep.avgQualityScore = rep.avgQualityScore * (1.0 - alpha) + qualityScore * alpha;
        rep.avgResponseTime = rep.avgResponseTime * (1.0 - alpha) + 
                             responseTime.count() / 1000.0 * alpha;
        
        rep.lastUpdated = std::chrono::system_clock::now();
        rep.trustScore = rep.score();
    }
    
    void recordByzantineBehavior(const std::string& peerId, const std::string& reason) override {
        (void)reason;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.byzantineBehaviors++;
        rep.trustScore *= 0.5; // Severe penalty
        rep.lastUpdated = std::chrono::system_clock::now();
    }
    
    void rewardNode(const std::string& peerId, double reputationBoost) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.trustScore = std::min(1.0, rep.trustScore + reputationBoost);
        rep.lastUpdated = std::chrono::system_clock::now();
    }
    
    void slashNode(const std::string& peerId, double reputationPenalty, uint64_t tokenSlash) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.trustScore = std::max(0.0, rep.trustScore - reputationPenalty);
        rep.totalSlashings += tokenSlash;
        rep.lastUpdated = std::chrono::system_clock::now();
    }
    
    std::vector<std::string> getTopNodes(std::size_t n) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<std::string, double>> scored;
        for (const auto& [peerId, rep] : reputations_) {
            scored.emplace_back(peerId, rep.score());
        }
        std::sort(scored.begin(), scored.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<std::string> result;
        for (std::size_t i = 0; i < std::min(n, scored.size()); ++i) {
            result.push_back(scored[i].first);
        }
        return result;
    }
    
    std::vector<std::string> getNodesAboveThreshold(double threshold) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> result;
        for (const auto& [peerId, rep] : reputations_) {
            if (rep.score() >= threshold) {
                result.push_back(peerId);
            }
        }
        return result;
    }
    
    std::unordered_map<std::string, Reputation> getAllReputations() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return reputations_;
    }
    
    void decayInactiveNodes(std::chrono::seconds inactivityThreshold) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        for (auto& [peerId, rep] : reputations_) {
            auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(
                now - rep.lastUpdated);
            if (timeSinceUpdate > inactivityThreshold) {
                rep.decay(timeSinceUpdate);
            }
        }
    }
    
    bool resetReputation(const std::string& peerId, const std::string& reason) override {
        (void)reason;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = reputations_.find(peerId);
        if (it != reputations_.end()) {
            it->second = Reputation{};
            it->second.peerId = peerId;
            it->second.trustScore = 0.5;
            return true;
        }
        return false;
    }
    
    std::vector<std::string> exportReputationLog() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> log;
        for (const auto& [peerId, rep] : reputations_) {
            log.push_back("PeerID: " + peerId + ", Score: " + std::to_string(rep.score()) +
                         ", Tasks: " + std::to_string(rep.totalTasks) +
                         ", Success: " + std::to_string(rep.successfulTasks));
        }
        return log;
    }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Reputation> reputations_;
};

class LatencyMap final : public ILatencyMap {
public:
    LatencyMap() = default;
    
    std::optional<double> getLatencyMs(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = latencies_.find(peerId);
        if (it != latencies_.end()) {
            return it->second.latencyMs;
        }
        return std::nullopt;
    }
    
    void updateLatency(const std::string& peerId, double latencyMs) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = latencies_[peerId];
        entry.latencyMs = latencyMs;
        entry.lastUpdated = std::chrono::system_clock::now();
    }
    
    std::optional<double> getBandwidthMbps(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = latencies_.find(peerId);
        if (it != latencies_.end()) {
            return it->second.bandwidthMbps;
        }
        return std::nullopt;
    }
    
    std::optional<double> getJitterMs(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = latencies_.find(peerId);
        if (it != latencies_.end()) {
            return it->second.jitterMs;
        }
        return std::nullopt;
    }
    
    std::optional<double> probeLatency(const std::string& peerId) override {
        // In production, this would send actual network probes
        // For now, return cached value or simulate
        auto cached = getLatencyMs(peerId);
        if (cached) return cached;
        
        // Simulate probe (would be real network ping in production)
        double simulatedLatency = 50.0 + (rand() % 100);
        updateLatency(peerId, simulatedLatency);
        return simulatedLatency;
    }
    
    std::optional<double> getDistanceKm(const std::string& peerId) const override {
        (void)peerId;
        // Would use geographic coordinates in production
        return std::nullopt;
    }
    
    std::unordered_map<std::string, double> getAllLatencies() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_map<std::string, double> result;
        for (const auto& [peerId, entry] : latencies_) {
            result[peerId] = entry.latencyMs;
        }
        return result;
    }
    
    void cleanupStale(std::chrono::seconds maxAge) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        for (auto it = latencies_.begin(); it != latencies_.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.lastUpdated);
            if (age > maxAge) {
                it = latencies_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
private:
    struct LatencyEntry {
        double latencyMs = 0.0;
        double bandwidthMbps = 0.0;
        double jitterMs = 0.0;
        std::chrono::system_clock::time_point lastUpdated;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, LatencyEntry> latencies_;
};

// ==================== ORCHESTRATOR ====================

class WeightedOrchestrator final : public IOrchestrator {
public:
    using ScorerFn = std::function<double(const NodeMetrics&, const TaskPayload&)>;

    WeightedOrchestrator(IReputationLedger& rep, ILatencyMap& lat)
        : rep_(rep), lat_(lat) {}

    Assignment assignBestWorker(const TaskPayload& task,
                                const std::vector<NodeMetrics>& candidates,
                                double trustW,
                                double speedW,
                                double powerW) const override;
    Assignment assignWithStrategy(const TaskPayload& task,
                                  const std::vector<NodeMetrics>& candidates,
                                  SchedulingStrategy strategy) const override;

    void setStrategy(SchedulingStrategy strategy) override { strategy_ = strategy; }
    SchedulingStrategy getStrategy() const override { return strategy_; }

    OrchestratorMetrics getMetrics() const override { return metrics_; }

    void setCustomScorer(ScorerFn scorer) { customScorer_ = std::move(scorer); }

    std::vector<Assignment> assignParallel(const std::vector<TaskPayload>& tasks,
                                           const std::vector<NodeMetrics>& candidates) const;
    std::vector<std::pair<std::string, double>> rankCandidates(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates) const;
    std::optional<Assignment> findBackupWorker(const TaskPayload& task,
                                               const std::vector<NodeMetrics>& candidates,
                                               const std::string& excludePeerId) const;
    std::vector<Assignment> scheduleBatch(const std::vector<TaskPayload>& tasks,
                                          const std::vector<NodeMetrics>& candidates) const;
    std::vector<std::pair<std::string, std::string>> rebalanceTasks(
        const std::vector<TaskPayload>& tasks,
        const std::vector<NodeMetrics>& candidates) const;
    std::optional<Assignment> findCheapestWorker(const TaskPayload& task,
                                                 const std::vector<NodeMetrics>& candidates) const;
    uint64_t estimateCost(const TaskPayload& task, const NodeMetrics& worker) const;
    Assignment optimizeCostPerformance(const TaskPayload& task,
                                       const std::vector<NodeMetrics>& candidates) const;
    Assignment assignRoundRobin(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignLeastLoaded(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignLowestLatency(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignHighestReputation(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignLowestCost(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignGeneticAlgorithm(const TaskPayload& task,
                                      const std::vector<NodeMetrics>& candidates) const;

private:
    double scoreNode(const NodeMetrics& node,
                     const TaskPayload& task,
                     double trustW,
                     double speedW,
                     double powerW) const;
    std::vector<NodeMetrics> filterCandidates(const std::vector<NodeMetrics>& candidates,
                                              const TaskPayload& task) const;

    IReputationLedger& rep_;
    ILatencyMap& lat_;
    SchedulingStrategy strategy_{SchedulingStrategy::WEIGHTED_SCORE};
    mutable OrchestratorMetrics metrics_{};
    std::optional<ScorerFn> customScorer_;
};

// ==================== TASK QUEUE ====================

class TaskQueue {
public:
    void enqueue(const TaskPayload& task, TaskPriority priority = TaskPriority::NORMAL) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (priority == TaskPriority::CRITICAL) {
            criticalQueue_.push(task);
        } else if (priority == TaskPriority::HIGH) {
            highQueue_.push(task);
        } else if (priority == TaskPriority::NORMAL) {
            normalQueue_.push(task);
        } else {
            lowQueue_.push(task);
        }
        cv_.notify_one();
    }
    
    std::optional<TaskPayload> dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return !criticalQueue_.empty() || !highQueue_.empty() ||
                   !normalQueue_.empty() || !lowQueue_.empty() || shutdown_;
        });
        
        if (shutdown_) return std::nullopt;
        
        if (!criticalQueue_.empty()) {
            auto task = criticalQueue_.front();
            criticalQueue_.pop();
            return task;
        }
        if (!highQueue_.empty()) {
            auto task = highQueue_.front();
            highQueue_.pop();
            return task;
        }
        if (!normalQueue_.empty()) {
            auto task = normalQueue_.front();
            normalQueue_.pop();
            return task;
        }
        if (!lowQueue_.empty()) {
            auto task = lowQueue_.front();
            lowQueue_.pop();
            return task;
        }
        
        return std::nullopt;
    }
    
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return criticalQueue_.size() + highQueue_.size() + 
               normalQueue_.size() + lowQueue_.size();
    }

    std::vector<TaskPayload> snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<TaskPayload> tasks;
        tasks.reserve(criticalQueue_.size() + highQueue_.size() +
                      normalQueue_.size() + lowQueue_.size());
        auto collect = [&tasks](std::queue<TaskPayload> queueCopy) {
            while (!queueCopy.empty()) {
                tasks.push_back(queueCopy.front());
                queueCopy.pop();
            }
        };
        collect(criticalQueue_);
        collect(highQueue_);
        collect(normalQueue_);
        collect(lowQueue_);
        return tasks;
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        cv_.notify_all();
    }
    
private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<TaskPayload> criticalQueue_;
    std::queue<TaskPayload> highQueue_;
    std::queue<TaskPayload> normalQueue_;
    std::queue<TaskPayload> lowQueue_;
    bool shutdown_ = false;
};

// ==================== ENGINE ====================

class Engine {
public:
    explicit Engine(const Config& config)
        : config_(config),
          repLedger_(),
          latencyMap_(),
          orchestrator_(repLedger_, latencyMap_),
          running_(false)
    {
        orchestrator_.setStrategy(config_.performance.defaultStrategy);

        std::string swarm_err;
        proverSwarm_ = std::make_unique<ailee::orchestration::ProverSwarm>(ailee::orchestration::ProverSwarmConfig());
        if (proverSwarm_ && !proverSwarm_->initialize(&swarm_err)) {
            proverSwarm_.reset();
        }
    }
    
    ~Engine() {
        stop();
    }
    
    // ========== Lifecycle ==========
    
    void start() {
        if (running_.exchange(true)) {
            return; // Already running
        }
        
        // Start worker threads
        for (uint32_t i = 0; i < config_.performance.workerThreads; ++i) {
            workerThreads_.emplace_back(&Engine::workerLoop, this);
        }
        
        // Start discovery thread
        discoveryThread_ = std::thread(&Engine::discoveryLoop, this);
        
        // Start monitoring thread
        if (config_.monitoring.enableMetrics) {
            monitoringThread_ = std::thread(&Engine::monitoringLoop, this);
        }
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }
        
        taskQueue_.shutdown();
        
        // Join worker threads
        for (auto& thread : workerThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        workerThreads_.clear();
        
        // Join discovery thread
        if (discoveryThread_.joinable()) {
            discoveryThread_.join();
        }
        
        // Join monitoring thread
        if (monitoringThread_.joinable()) {
            monitoringThread_.join();
        }

        failAllPending("Engine stopped before assignment");
    }
    
    bool isRunning() const {
        return running_.load();
    }
    
    // ========== Task Submission ==========
    
    std::future<Assignment> submitTask(const TaskPayload& task) {
        auto promise = std::make_shared<std::promise<Assignment>>();
        auto future = promise->get_future();
        
        taskQueue_.enqueue(task, task.priority);
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingPromises_[task.taskId] = promise;
        }
        
        return future;
    }
    
    void runTask(const TaskPayload& task) {
        auto candidates = discoverNodes();
        
        if (candidates.empty()) {
            // No nodes available
            totalTasksFailed_++;
            resolvePromise(task.taskId,
                           makeFailureAssignment(task, "No available nodes for task"));
            return;
        }
        
        auto assignment = orchestrator_.assignBestWorker(
            task, candidates,
            config_.performance.trustWeight,
            config_.performance.speedWeight,
            config_.performance.powerWeight
        );
        
        if (assignment.assigned) {
            dispatch(assignment, task);
        } else {
            totalTasksFailed_++;
            resolvePromise(task.taskId, assignment);
        }
    }
    
    // ========== Node Management ==========
    
    void registerNode(const NodeMetrics& node) {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        knownNodes_[node.peerId] = node;
    }
    
    void unregisterNode(const std::string& peerId) {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        knownNodes_.erase(peerId);
    }
    
    std::vector<NodeMetrics> getNodes() const {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        std::vector<NodeMetrics> nodes;
        nodes.reserve(knownNodes_.size());
        for (const auto& [id, node] : knownNodes_) {
            nodes.push_back(node);
        }
        return nodes;
    }
    
    // ========== Configuration ==========
    
    void updateConfig(const Config& config) {
        config_ = config;
        orchestrator_.setStrategy(config_.performance.defaultStrategy);
    }
    
    const Config& getConfig() const {
        return config_;
    }
    
    // ========== Telemetry ==========
    
    struct EngineMetrics {
        uint64_t totalTasksSubmitted = 0;
        uint64_t totalTasksCompleted = 0;
        uint64_t totalTasksFailed = 0;
        uint64_t activeNodes = 0;
        std::size_t queuedTasks = 0;
        std::chrono::milliseconds avgTaskLatency{0};
        IOrchestrator::OrchestratorMetrics orchestratorMetrics;
    };
    
    EngineMetrics getMetrics() const {
        EngineMetrics metrics;
        metrics.totalTasksSubmitted = totalTasksSubmitted_.load();
        metrics.totalTasksCompleted = totalTasksCompleted_.load();
        metrics.totalTasksFailed = totalTasksFailed_.load();
        metrics.queuedTasks = taskQueue_.size();
        metrics.orchestratorMetrics = orchestrator_.getMetrics();
        
        std::lock_guard<std::mutex> lock(nodesMutex_);
        metrics.activeNodes = knownNodes_.size();
        
        return metrics;
    }
    
    // ========== Access to Components ==========
    
    IReputationLedger& getReputationLedger() { return repLedger_; }
    ILatencyMap& getLatencyMap() { return latencyMap_; }
    IOrchestrator& getOrchestrator() { return orchestrator_; }
    std::vector<TaskPayload> getQueuedTasks() const { return taskQueue_.snapshot(); }
    
    ailee::orchestration::ProverSwarm* getProverSwarm() const { return proverSwarm_.get(); }

private:
    Config config_;
    ReputationLedger repLedger_;
    LatencyMap latencyMap_;
    WeightedOrchestrator orchestrator_;
    std::unique_ptr<ailee::orchestration::ProverSwarm> proverSwarm_;
    
    std::atomic<bool> running_;
    TaskQueue taskQueue_;
    
    mutable std::mutex nodesMutex_;
    std::unordered_map<std::string, NodeMetrics> knownNodes_;
    
    std::vector<std::thread> workerThreads_;
    std::thread discoveryThread_;
    std::thread monitoringThread_;
    
    std::unordered_map<std::string, std::shared_ptr<std::promise<Assignment>>> pendingPromises_;
    mutable std::mutex pendingMutex_;
    
    std::atomic<uint64_t> totalTasksSubmitted_{0};
    std::atomic<uint64_t> totalTasksCompleted_{0};
    std::atomic<uint64_t> totalTasksFailed_{0};
    
    // ========== Worker Loop ==========
    
    void workerLoop() {
        while (running_.load()) {
            auto taskOpt = taskQueue_.dequeue();
            if (!taskOpt) break; // Shutdown
            
            totalTasksSubmitted_++;
            
            try {
                runTask(*taskOpt);
            } catch (const std::exception& e) {
                totalTasksFailed_++;
                resolvePromise(taskOpt->taskId,
                               makeFailureAssignment(*taskOpt,
                                                    "Task processing error: " + std::string(e.what())));
            }
        }
    }
    
    // ========== Discovery Loop ==========
    
    void discoveryLoop() {
        while (running_.load()) {
            try {
                // Discover new nodes (would use libp2p or similar in production)
                auto newNodes = performDiscovery();
                
                for (const auto& node : newNodes) {
                    registerNode(node);
                }
                
                // Update latency measurements
                updateLatencies();
                
                // Decay inactive reputations
                repLedger_.decayInactiveNodes(std::chrono::hours(24));
                
            } catch (const std::exception& e) {
                // Log error
            }
            
            std::this_thread::sleep_for(config_.discoveryInterval);
        }
    }
    
    // ========== Monitoring Loop ==========
    
    void monitoringLoop() {
        while (running_.load()) {
            try {
                auto metrics = getMetrics();
                // Export metrics (would send to Prometheus/etc in production)
                
            } catch (const std::exception& e) {
                // Log error
            }
            
            std::this_thread::sleep_for(config_.monitoring.metricsInterval);
        }
    }
    
    // ========== Helper Methods ==========
    
    std::vector<NodeMetrics> discoverNodes() const {
        return getNodes();
    }
    
    std::vector<NodeMetrics> performDiscovery() {
        // In production, this would query DHT, bootstrap peers, etc.
        // For now, return empty vector
        return {};
    }
    
    void updateLatencies() {
        auto nodes = getNodes();
        for (const auto& node : nodes) {
            latencyMap_.probeLatency(node.peerId);
        }
    }
    
    void dispatch(const Assignment& assignment, const TaskPayload& task) {
        // In production, this would send task to worker node
        // For now, simulate success
        
        totalTasksCompleted_++;
        
        // Update reputation
        bool success = assignment.assigned;
        repLedger_.recordTaskCompletion(
            assignment.workerPeerId,
            success,
            1.0, // quality score
            std::chrono::milliseconds(100)
        );
        
        // Resolve promise if exists
        resolvePromise(task.taskId, assignment);
    }

    Assignment makeFailureAssignment(const TaskPayload& task, const std::string& reason) const {
        Assignment assignment;
        assignment.assigned = false;
        assignment.reason = reason;
        assignment.assignmentId = task.taskId + "-failed";
        assignment.assignedAt = std::chrono::system_clock::now();
        return assignment;
    }

    void resolvePromise(const std::string& taskId, const Assignment& assignment) {
        std::shared_ptr<std::promise<Assignment>> promise;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            auto it = pendingPromises_.find(taskId);
            if (it == pendingPromises_.end()) {
                return;
            }
            promise = std::move(it->second);
            pendingPromises_.erase(it);
        }

        if (promise) {
            try {
                promise->set_value(assignment);
            } catch (const std::future_error&) {
                // Promise already satisfied or broken; ignore.
            }
        }
    }

    void failAllPending(const std::string& reason) {
        std::unordered_map<std::string, std::shared_ptr<std::promise<Assignment>>> pending;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pending.swap(pendingPromises_);
        }

        for (auto& [taskId, promise] : pending) {
            if (!promise) {
                continue;
            }
            Assignment assignment;
            assignment.assigned = false;
            assignment.reason = reason;
            assignment.assignmentId = taskId + "-cancelled";
            assignment.assignedAt = std::chrono::system_clock::now();
            try {
                promise->set_value(assignment);
            } catch (const std::future_error&) {
                // Promise already satisfied or broken; ignore.
            }
        }
    }
};

// ==================== FACTORY FUNCTIONS ====================

inline std::unique_ptr<Engine> createEngine(const Config& config) {
    return std::make_unique<Engine>(config);
}

inline Config createDefaultConfig() {
    Config config;
    config.performance.defaultStrategy = SchedulingStrategy::WEIGHTED_SCORE;
    config.performance.maxConcurrentTasks = 100;
    auto cores = std::thread::hardware_concurrency();
    config.performance.workerThreads = cores == 0 ? 1 : cores;
    config.economic.defaultMaxCostTokens = 1000;
    config.economic.minReputationThreshold = 0.5;
    return config;
}

} // namespace ailee::sched
