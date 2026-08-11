// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
// WasmEdgeEngine.h — Production WASM Engine using WasmEdge Runtime
// Real sandboxed execution with resource limits and metrics collection

#pragma once

#include "WasmEngine.h"
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <stdexcept>

// Forward declarations for WasmEdge types (to avoid including full SDK in header)
struct WasmEdge_ConfigureContext;
struct WasmEdge_VMContext;
struct WasmEdge_ModuleInstanceContext;
struct WasmEdge_Result;

namespace ailee::exec {

/**
 * Production-grade WASM execution engine using WasmEdge runtime.
 * 
 * Features:
 * - Full resource isolation (memory, CPU, time)
 * - Gas metering for cost control
 * - Deterministic execution
 * - Execution trace recording for ZK proofs
 * - Multi-module caching
 * - Comprehensive error handling
 * 
 * Security:
 * - No filesystem access by default
 * - No network access by default
 * - Memory limits enforced by runtime
 * - Timeout kills via WASI
 * 
 * Performance:
 * - Near-native execution speed (~1.5x slowdown vs native)
 * - AOT compilation support for hot modules
 * - Lazy module loading
 */
class WasmEdgeEngine {
public:
    /**
     * Initialize WasmEdge engine with default configuration
     */
    WasmEdgeEngine();
    
    /**
     * Initialize with custom limits
     */
    explicit WasmEdgeEngine(const SandboxLimits& limits);
    
    ~WasmEdgeEngine();
    
    // Disable copy (VM context is not copyable)
    WasmEdgeEngine(const WasmEdgeEngine&) = delete;
    WasmEdgeEngine& operator=(const WasmEdgeEngine&) = delete;
    
    // Enable move
    WasmEdgeEngine(WasmEdgeEngine&&) noexcept;
    WasmEdgeEngine& operator=(WasmEdgeEngine&&) noexcept;
    
    /**
     * Execute a WASM function call
     * 
     * @param call Function call specification
     * @return Execution result with output and metrics
     * @throws WasmExecutionException on unrecoverable errors
     */
    WasmResult execute(const WasmCall& call);
    
    /**
     * Execute with execution trace recording for ZK proof generation
     * 
     * @param call Function call specification
     * @return Result with execution trace
     */
    WasmResult executeWithTrace(const WasmCall& call);
    
    /**
     * Load and cache a WASM module
     * 
     * @param moduleBytes WASM bytecode
     * @param moduleHash SHA3-256 hash of bytecode
     * @return true if loaded successfully
     */
    bool loadModule(const std::vector<uint8_t>& moduleBytes, const std::string& moduleHash);
    
    /**
     * Unload a cached module
     */
    void unloadModule(const std::string& moduleHash);
    
    /**
     * Check if execution is deterministic (same inputs → same outputs)
     * 
     * @param moduleHash Module to test
     * @param testInputs Input bytes for determinism test
     * @param iterations Number of times to execute (default: 3)
     * @return true if all executions produce identical output
     */
    bool verifyDeterminism(const std::string& moduleHash, 
                          const std::vector<uint8_t>& testInputs,
                          int iterations = 3);
    
    /**
     * Get current configuration
     */
    SandboxLimits getLimits() const { return limits_; }
    
    /**
     * Update runtime limits (affects future executions only)
     */
    void setLimits(const SandboxLimits& limits);
    
    /**
     * Get execution statistics since engine creation
     */
    struct Statistics {
        uint64_t totalExecutions = 0;
        uint64_t successfulExecutions = 0;
        uint64_t timeoutErrors = 0;
        uint64_t memoryErrors = 0;
        uint64_t otherErrors = 0;
        std::chrono::microseconds totalExecutionTime{0};
        std::chrono::microseconds averageExecutionTime{0};
    };
    
    Statistics getStatistics() const { return stats_; }
    
    /**
     * Reset statistics counters
     */
    void resetStatistics();

private:
    // WasmEdge runtime contexts
    WasmEdge_ConfigureContext* config_ = nullptr;
    WasmEdge_VMContext* vm_ = nullptr;
    
    // Configuration
    SandboxLimits limits_;
    
    // Module cache (moduleHash → ModuleInstance)
    struct CachedModule {
        std::vector<uint8_t> bytecode;
        WasmEdge_ModuleInstanceContext* instance;
        std::chrono::system_clock::time_point loadedAt;
        uint64_t executionCount = 0;
    };
    std::unordered_map<std::string, CachedModule> moduleCache_;
    std::mutex cacheMutex_;
    
    // Statistics
    mutable std::mutex statsMutex_;
    Statistics stats_;
    
    // Internal helpers
    void initializeVM();
    void destroyVM();
    void configureResourceLimits();
    
    WasmResult executeInternal(const WasmCall& call, bool recordTrace);
    
    WasmEdge_ModuleInstanceContext* getOrLoadModule(const std::string& moduleHash);
    
    void recordMetrics(const WasmResult& result);
    
    std::string wasmedgeResultToString(const WasmEdge_Result& result);
    
    // Execution trace recording
    struct ExecutionTrace {
        std::vector<std::string> opcodes;
        std::vector<uint64_t> memoryAccesses;
        std::vector<uint64_t> stackOperations;
    };
    
    ExecutionTrace recordTrace_;
    bool traceEnabled_ = false;
};

/**
 * Exception thrown on WASM execution errors
 */
class WasmExecutionException : public std::runtime_error {
public:
    enum class ErrorCode {
        MODULE_NOT_FOUND,
        MODULE_LOAD_FAILED,
        FUNCTION_NOT_FOUND,
        EXECUTION_FAILED,
        TIMEOUT,
        MEMORY_LIMIT_EXCEEDED,
        GAS_LIMIT_EXCEEDED,
        INVALID_INPUT,
        INVALID_OUTPUT,
        DETERMINISM_VIOLATION
    };
    
    WasmExecutionException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}
    
    ErrorCode code() const { return code_; }
    
private:
    ErrorCode code_;
};

} // namespace ailee::exec
