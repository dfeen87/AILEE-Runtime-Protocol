// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
// DistributedTaskProtocol.h — Task distribution protocol for multi-node coordination

#pragma once

#include "P2PNetwork.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <chrono>

namespace ailee::orchestration {

/**
 * Task types supported by the protocol
 */
enum class TaskType {
    COMPUTATION,      // General computation task
    FEDERATED_LEARNING, // Federated learning round
    VERIFICATION,     // Proof verification
    STORAGE,          // Data storage/retrieval
    CUSTOM            // Custom task type
};

/**
 * Task priority levels
 */
enum class TaskPriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
};

/**
 * Distributed task structure
 */
struct DistributedTask {
    std::string taskId;           // Unique task identifier
    TaskType type;                // Task type
    TaskPriority priority;        // Task priority
    std::string originNode;       // Node that created the task
    std::vector<uint8_t> payload; // Task payload (serialized)
    uint64_t createdAt;           // Creation timestamp
    uint64_t deadline;            // Deadline timestamp (0 = no deadline)
    uint32_t retryCount;          // Number of retries attempted
    uint32_t maxRetries;          // Maximum retries allowed
    
    // Resource requirements
    struct Requirements {
        uint64_t minMemoryMB = 0;
        uint32_t minCpuCores = 0;
        uint64_t estimatedDurationMs = 0;
        bool requiresGPU = false;
    } requirements;
};

/**
 * Task result structure
 */
struct TaskResult {
    std::string taskId;           // Associated task ID
    std::string executorNode;     // Node that executed the task
    bool success;                 // Execution success status
    std::vector<uint8_t> result;  // Task result (serialized)
    std::string errorMessage;     // Error message (if failed)
    uint64_t completedAt;         // Completion timestamp
    uint64_t executionTimeMs;     // Actual execution time
    
    // Proof of execution (optional)
    std::string proofHash;        // Hash of execution proof
};

/**
 * Task event types for callbacks
 */
enum class TaskEvent {
    RECEIVED,     // Task received from network
    STARTED,      // Task execution started
    COMPLETED,    // Task execution completed
    FAILED,       // Task execution failed
    TIMEOUT,      // Task deadline exceeded
    CANCELLED     // Task cancelled
};

/**
 * Task event callback type
 */
using TaskEventCallback = std::function<void(const std::string& taskId, TaskEvent event, const std::string& details)>;

/**
 * Task executor callback type
 * Returns task result or nullopt on failure
 */
using TaskExecutor = std::function<std::optional<TaskResult>(const DistributedTask&)>;

/**
 * Distributed Task Protocol
 * 
 * Provides:
 * - Task distribution across P2P network
 * - Task execution coordination
 * - Result aggregation
 * - Fault tolerance with retries
 * - Priority-based scheduling
 */
class DistributedTaskProtocol {
public:
    struct Config {
        std::string nodeId;                    // Local node identifier
        uint32_t maxConcurrentTasks;           // Max parallel tasks
        uint32_t taskTimeoutSeconds;           // Default task timeout
        bool autoExecute;                      // Auto-execute received tasks
        std::string resultsTopicPrefix;
        std::string tasksTopicPrefix;
        
        Config()
            : maxConcurrentTasks(10)
            , taskTimeoutSeconds(300)
            , autoExecute(true)
            , resultsTopicPrefix("ailee/task/results")
            , tasksTopicPrefix("ailee/task/distribute")
        {}
    };
    
    explicit DistributedTaskProtocol(
        std::shared_ptr<network::P2PNetwork> network,
        const Config& config = Config()
    );
    ~DistributedTaskProtocol();
    
    // Disable copy, allow move
    DistributedTaskProtocol(const DistributedTaskProtocol&) = delete;
    DistributedTaskProtocol& operator=(const DistributedTaskProtocol&) = delete;
    DistributedTaskProtocol(DistributedTaskProtocol&&) = default;
    DistributedTaskProtocol& operator=(DistributedTaskProtocol&&) = default;
    
    /**
     * Start the protocol
     */
    bool start();
    
    /**
     * Stop the protocol
     */
    void stop();
    
    /**
     * Check if protocol is running
     */
    bool isRunning() const;
    
    /**
     * Distribute a task to the network
     * @param task Task to distribute
     * @return true if successfully sent
     */
    bool distributeTask(const DistributedTask& task);
    
    /**
     * Execute a task locally
     * @param taskId Task identifier
     * @return Task result
     */
    std::optional<TaskResult> executeTask(const std::string& taskId);
    
    /**
     * Cancel a task
     */
    bool cancelTask(const std::string& taskId);
    
    /**
     * Get task status
     */
    enum class TaskStatus {
        UNKNOWN,
        PENDING,
        RUNNING,
        COMPLETED,
        FAILED,
        CANCELLED
    };
    TaskStatus getTaskStatus(const std::string& taskId) const;
    
    /**
     * Get task result (if available)
     */
    std::optional<TaskResult> getTaskResult(const std::string& taskId) const;
    
    /**
     * Register task executor for a task type
     */
    void registerExecutor(TaskType type, TaskExecutor executor);
    
    /**
     * Register event callback
     */
    void setEventCallback(TaskEventCallback callback);
    
    /**
     * Get protocol statistics
     */
    struct ProtocolStats {
        uint32_t tasksSent;
        uint32_t tasksReceived;
        uint32_t tasksExecuted;
        uint32_t tasksFailed;
        uint32_t currentPendingTasks;
        uint32_t currentRunningTasks;
        double avgExecutionTimeMs;
    };
    ProtocolStats getStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ailee::orchestration
