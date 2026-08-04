// Licensed under the PolyForm Noncommercial License 1.0.0
// WasmEngine.h — Production-Grade WASM Execution Engine for AILEE-Core
// Secure sandboxed execution with resource limits, telemetry, ZK proof integration,
// and multi-runtime support for decentralized AI inference workloads.

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>
#include <memory>
#include <functional>
#include <atomic>

namespace ailee::exec {

// ==================== RESOURCE LIMITS & SECURITY ====================

struct SandboxLimits {
    std::size_t memoryBytes = 512 * 1024 * 1024;  // 512MB default
    std::size_t maxStackBytes = 1 * 1024 * 1024;   // 1MB stack
    std::chrono::milliseconds timeout{30000};      // 30s execution limit
    std::chrono::milliseconds startupTimeout{5000}; // 5s instantiation limit
    
    // Capability-based permissions
    bool allowFilesystem = false;
    bool allowNetwork = false;
    bool allowSystemAPIs = false;
    bool allowThreads = false;
    bool allowCrypto = true;  // Allow cryptographic operations
    
    // Resource metering
    uint64_t maxInstructions = 10'000'000'000;  // 10B instructions max
    uint64_t maxFunctionCalls = 1'000'000;      // 1M nested calls max
    uint32_t maxTableSize = 10'000;             // WASM table limit
    uint32_t maxMemoryPages = 8192;             // 512MB / 64KB pages
    
    // Gas metering for economic control
    bool enableGasMetering = true;
    uint64_t gasLimit = 1'000'000'000;          // 1B gas units
    double gasPricePerInstruction = 0.001;      // Cost per instruction
};

// ==================== EXECUTION TELEMETRY ====================

struct ExecutionMetrics {
    std::chrono::microseconds executionTime{0};
    std::chrono::microseconds instantiationTime{0};
    std::size_t peakMemoryUsed = 0;
    std::size_t averageMemoryUsed = 0;
    uint64_t instructionsExecuted = 0;
    uint64_t gasConsumed = 0;
    uint32_t functionCallCount = 0;
    bool timeoutTriggered = false;
    bool memoryLimitExceeded = false;
    bool gasLimitExceeded = false;
    std::string runtimeVersion;
};

// ==================== WASM CALL INTERFACE ====================

struct WasmCall {
    std::string functionName;             // e.g., "run_inference", "train_local"
    std::vector<uint8_t> inputBytes;      // Canonical CBOR/MessagePack/Protobuf
    
    // Optional parameters
    std::optional<uint64_t> customGasLimit;
    std::optional<std::chrono::milliseconds> customTimeout;
    std::optional<std::string> nodeId;     // For telemetry tracking
    
    // Cryptographic verification
    std::string inputHash;                 // SHA3-256 of inputBytes
    std::optional<std::string> signatureProof; // Optional ZK proof
};

// ==================== WASM RESULT WITH PROOF ====================

struct WasmResult {
    bool success = false;
    std::vector<uint8_t> outputBytes;     // Encoded inference result
    std::string outputHash;                // SHA3-256 of outputBytes
    
    // Determinism & reproducibility
    std::string moduleHash;                // SHA3-256 of WASM bytecode
    std::string executionHash;             // Hash of (moduleHash + inputHash + outputHash)
    
    // Error handling
    std::string error;                     // Detailed error message
    std::optional<uint32_t> errorCode;     // Structured error codes
    std::optional<std::string> stackTrace; // Debug information (if enabled)
    
    // Telemetry
    ExecutionMetrics metrics;
    
    // Zero-Knowledge Proof Integration
    std::optional<std::string> zkProof;    // Proof of correct execution
    bool zkVerified = false;               // Whether proof was verified
    
    // Timestamp for replay protection
    std::chrono::system_clock::time_point timestamp;
};

// ==================== ERROR CODES ====================

enum class WasmErrorCode : uint32_t {
    SUCCESS = 0,
    MODULE_LOAD_FAILED = 1,
    INSTANTIATION_FAILED = 2,
    FUNCTION_NOT_FOUND = 3,
    EXECUTION_TIMEOUT = 4,
    MEMORY_LIMIT_EXCEEDED = 5,
    GAS_LIMIT_EXCEEDED = 6,
    INVALID_INPUT = 7,
    INVALID_OUTPUT = 8,
    TRAP_OCCURRED = 9,
    SECURITY_VIOLATION = 10,
    DETERMINISM_VIOLATION = 11,
    UNKNOWN_ERROR = 255
};

// ==================== WASM ENGINE INTERFACE ====================

class IWasmEngine {
public:
    virtual ~IWasmEngine() = default;
    
    // ========== Core Lifecycle ==========
    
    // Load and validate WASM module from bytes (IPFS/P2P fetch happens upstream)
    // Returns true on success, populates err on failure
    virtual bool loadModule(const std::vector<uint8_t>& wasmModuleBytes,
                            std::string* err) = 0;
    
    // Configure hard isolation limits before instantiation
    virtual void configure(const SandboxLimits& limits) = 0;
    
    // Get current configuration
    virtual SandboxLimits getConfiguration() const = 0;
    
    // Execute function with strict confinement; returns output + proof
    virtual WasmResult execute(const WasmCall& call) = 0;
    
    // Immediately tear down VM and release all resources
    virtual void destroy() noexcept = 0;
    
    // ========== Advanced Features ==========
    
    // Pre-compile module for faster instantiation (JIT optimization)
    virtual bool precompile(std::string* err) = 0;
    
    // Validate module without instantiation (static analysis)
    virtual bool validate(std::string* err) = 0;
    
    // Get module metadata (exports, imports, memory requirements)
    virtual std::vector<std::string> getExportedFunctions() const = 0;
    
    // Check if module is deterministic (no non-deterministic imports)
    virtual bool isDeterministic() const = 0;
    
    // Get runtime-specific information
    virtual std::string getRuntimeInfo() const = 0;
    
    // ========== Security & Verification ==========
    
    // Enable determinism checking (enforce reproducible execution)
    virtual void enableDeterminismChecking(bool enable) = 0;
    
    // Generate ZK proof of execution (if supported by runtime)
    virtual std::optional<std::string> generateExecutionProof(
        const WasmCall& call,
        const WasmResult& result) = 0;
    
    // Verify ZK proof from another node
    virtual bool verifyExecutionProof(
        const std::string& proof,
        const std::string& moduleHash,
        const std::string& inputHash,
        const std::string& outputHash) = 0;
    
    // ========== Resource Management ==========
    
    // Get current memory usage
    virtual std::size_t getCurrentMemoryUsage() const = 0;
    
    // Get peak memory usage since load
    virtual std::size_t getPeakMemoryUsage() const = 0;
    
    // Force garbage collection (if applicable)
    virtual void collectGarbage() = 0;
    
    // Check if engine is healthy and responsive
    virtual bool isHealthy() const = 0;
    
    // ========== Telemetry & Monitoring ==========
    
    // Get aggregated metrics since module load
    virtual ExecutionMetrics getAggregatedMetrics() const = 0;
    
    // Reset metrics counters
    virtual void resetMetrics() = 0;
    
    // Set callback for execution events (optional monitoring)
    using EventCallback = std::function<void(const std::string& event, 
                                              const std::string& details)>;
    virtual void setEventCallback(EventCallback callback) = 0;
};

// ==================== RUNTIME IDENTIFIERS ====================

enum class WasmRuntime {
    WASMEDGE,   // WasmEdge (CNCF, production-grade)
    WASMER,     // Wasmer (Universal binaries)
    WAVM,       // WAVM (High-performance JIT)
    WASM3,      // Wasm3 (Lightweight interpreter)
    WASMTIME,   // Wasmtime (Bytecode Alliance)
    AUTO        // Automatic selection based on workload
};

// ==================== FACTORY & RUNTIME SELECTION ====================

class WasmEngineFactory {
public:
    // Create engine instance with specified runtime
    static std::unique_ptr<IWasmEngine> create(WasmRuntime runtime);
    
    // Create engine with automatic runtime selection
    static std::unique_ptr<IWasmEngine> createAuto(
        const std::vector<uint8_t>& wasmModuleBytes,
        const SandboxLimits& limits);
    
    // Get list of available runtimes on this platform
    static std::vector<WasmRuntime> getAvailableRuntimes();
    
    // Check if specific runtime is available
    static bool isRuntimeAvailable(WasmRuntime runtime);
    
    // Get runtime name as string
    static std::string getRuntimeName(WasmRuntime runtime);
    
    // Parse runtime from string (for CLI/config)
    static std::optional<WasmRuntime> parseRuntime(const std::string& name);
    
    // Get recommended runtime for workload type
    static WasmRuntime getRecommendedRuntime(const std::string& workloadType);
    
    // Benchmark runtimes and return performance ranking
    static std::vector<WasmRuntime> benchmarkRuntimes(
        const std::vector<uint8_t>& testModule,
        const WasmCall& testCall);
};

// ==================== CONVENIENCE UTILITIES ====================

namespace utils {
    // Compute SHA3-256 hash of bytes
    std::string computeHash(const std::vector<uint8_t>& data);
    
    // Verify hash matches data
    bool verifyHash(const std::vector<uint8_t>& data, const std::string& hash);
    
    // Encode result to CBOR/MessagePack
    std::vector<uint8_t> encodeResult(const WasmResult& result);
    
    // Decode result from CBOR/MessagePack
    std::optional<WasmResult> decodeResult(const std::vector<uint8_t>& data);
    
    // Convert error code to human-readable string
    std::string errorCodeToString(WasmErrorCode code);
    
    // Estimate gas cost for module (static analysis)
    uint64_t estimateGasCost(const std::vector<uint8_t>& wasmModuleBytes);
    
    // Check if module is safe (no suspicious imports)
    bool isSafeModule(const std::vector<uint8_t>& wasmModuleBytes);
}

// ==================== EXECUTION POOL (OPTIONAL) ====================

class WasmExecutionPool {
public:
    explicit WasmExecutionPool(size_t poolSize, WasmRuntime runtime);
    ~WasmExecutionPool();
    
    // Submit task for asynchronous execution
    using ResultCallback = std::function<void(WasmResult)>;
    void submitAsync(const std::vector<uint8_t>& moduleBytes,
                     const WasmCall& call,
                     const SandboxLimits& limits,
                     ResultCallback callback);
    
    // Execute synchronously (blocking)
    WasmResult executeSync(const std::vector<uint8_t>& moduleBytes,
                           const WasmCall& call,
                           const SandboxLimits& limits);
    
    // Get pool statistics
    struct PoolStats {
        size_t totalExecutions = 0;
        size_t activeWorkers = 0;
        size_t queuedTasks = 0;
        std::chrono::milliseconds avgExecutionTime{0};
        size_t failedExecutions = 0;
    };
    PoolStats getStats() const;
    
    // Shutdown pool and wait for pending tasks
    void shutdown();
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ailee::exec
