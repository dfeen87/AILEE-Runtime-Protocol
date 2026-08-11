// Licensed under the MIT License
// Copyright (c) 2026 Don Michael Feeney Jr.
// ZKVerifier.h — Production-Grade Zero-Knowledge Proof Verification for AILEE-Core
// Multi-backend support (RISC Zero, SP1, Groth16, PLONK, STARKs) with batching,
// caching, and cryptographic binding for trustless distributed AI computation.

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <functional>
#include <cstdint>
#include <unordered_map>

namespace ailee::zk {

// ==================== CRYPTOGRAPHIC PRIMITIVES ====================

enum class ProofSystem {
    RISC_ZERO,      // RISC Zero zkVM
    SP1,            // Succinct SP1 zkVM
    GROTH16,        // Groth16 SNARKs (fast verify, trusted setup)
    PLONK,          // PLONK SNARKs (universal setup)
    STARK,          // STARKs (no trusted setup, larger proofs)
    HALO2,          // Halo2 recursive SNARKs
    BULLETPROOFS,   // Bulletproofs (no trusted setup, slower)
    CUSTOM_ZKML,    // Custom zkML circuit (e.g., EZKL, Risc0-ML)
    AUTO            // Automatic selection based on proof metadata
};

enum class HashFunction {
    SHA256,
    SHA3_256,
    BLAKE3,
    POSEIDON,       // ZK-friendly hash
    KECCAK256
};

// ==================== PROOF BUNDLE ====================

struct ProofBundle {
    // Core proof data
    std::vector<uint8_t> proofBytes;      // Opaque proof (format depends on system)
    std::vector<uint8_t> publicInputs;    // Canonical encoding (CBOR/JSON/Protobuf)
    
    // Cryptographic binding
    std::string modelHash;                // SHA3-256 of WASM/model bytecode
    std::string inputHash;                // Hash of computation input
    std::string outputHash;               // Hash of computation output
    std::string executionHash;            // Hash(modelHash || inputHash || outputHash)
    
    // Metadata
    std::string taskId;                   // Unique task identifier
    std::string workerId;                 // Node that generated proof
    std::string circuitId;                // Circuit/program identifier
    ProofSystem proofSystem = ProofSystem::AUTO;
    
    // Versioning & replay protection
    uint32_t protocolVersion = 1;
    std::chrono::system_clock::time_point timestamp;
    std::optional<uint64_t> nonce;        // Anti-replay nonce
    
    // Optional signature (proof of prover identity)
    std::optional<std::vector<uint8_t>> proverSignature;
    std::optional<std::string> proverPubkey;
    
    // Performance metadata (for benchmarking/rewards)
    std::optional<std::chrono::milliseconds> proofGenTime;
    std::optional<std::size_t> proofSizeBytes;
    std::optional<uint64_t> gasConsumed;
};

// ==================== VERIFICATION KEY ====================

struct VerificationKey {
    std::string id;                       // Key identifier/version (e.g., "model-v1.2.3")
    std::vector<uint8_t> data;            // Opaque VK blob (depends on proof system)
    ProofSystem proofSystem;              // Which system this key is for
    HashFunction hashFunction = HashFunction::SHA3_256;
    
    // Cryptographic commitment
    std::string vkHash;                   // Hash of VK data (for integrity)
    std::optional<std::string> circuitCommitment; // Public commitment to circuit
    
    // Metadata
    std::chrono::system_clock::time_point createdAt;
    std::optional<std::chrono::system_clock::time_point> expiresAt;
    bool isTrustedSetup = false;          // Whether this requires trusted setup
    
    // Security parameters
    uint32_t securityBits = 128;          // Security level (128, 256)
    std::optional<std::string> setupCeremonyHash; // For auditing trusted setups
};

// ==================== VERIFICATION RESULT ====================

struct VerifyResult {
    bool verified = false;
    std::string reason;                   // Detailed explanation on failure
    
    // Verification metadata
    std::chrono::microseconds verificationTime{0};
    std::string verifierVersion;
    ProofSystem proofSystemUsed;
    
    // Cryptographic details
    bool signatureValid = true;           // If prover signature present
    bool timestampValid = true;           // If within acceptable window
    bool nonceValid = true;               // If nonce hasn't been seen before
    bool executionHashValid = false;      // If hash chain is correct
    
    // Public inputs extracted from proof
    std::optional<std::vector<uint8_t>> extractedPublicInputs;
    
    // Error codes for structured handling
    std::optional<uint32_t> errorCode;
};

// ==================== ERROR CODES ====================

enum class VerificationError : uint32_t {
    SUCCESS = 0,
    PROOF_INVALID = 1,
    VK_NOT_LOADED = 2,
    VK_MISMATCH = 3,
    PUBLIC_INPUT_MISMATCH = 4,
    EXECUTION_HASH_INVALID = 5,
    SIGNATURE_INVALID = 6,
    TIMESTAMP_EXPIRED = 7,
    NONCE_REPLAY_DETECTED = 8,
    PROOF_SYSTEM_UNSUPPORTED = 9,
    MALFORMED_PROOF = 10,
    CIRCUIT_MISMATCH = 11,
    SECURITY_LEVEL_INSUFFICIENT = 12,
    VERIFIER_ERROR = 255
};

// ==================== VERIFIER INTERFACE ====================

class IVerifier {
public:
    virtual ~IVerifier() = default;
    
    // ========== Core Verification ==========
    
    // Load verification key(s) or program commitments
    virtual bool loadKey(const VerificationKey& vk, std::string* err) = 0;
    
    // Verify single proof bundle (proof replaces trust)
    virtual VerifyResult verify(const ProofBundle& bundle) const = 0;
    
    // Batch verification (more efficient for multiple proofs)
    virtual std::vector<VerifyResult> verifyBatch(
        const std::vector<ProofBundle>& bundles) const = 0;
    
    // ========== Key Management ==========
    
    // Unload verification key (free memory)
    virtual bool unloadKey(const std::string& keyId) = 0;
    
    // Check if key is loaded
    virtual bool hasKey(const std::string& keyId) const = 0;
    
    // Get all loaded key IDs
    virtual std::vector<std::string> getLoadedKeys() const = 0;
    
    // Validate VK integrity (check hash)
    virtual bool validateKey(const VerificationKey& vk, std::string* err) const = 0;
    
    // ========== Advanced Features ==========
    
    // Pre-compile verification circuit (optimization)
    virtual bool precompileCircuit(const std::string& circuitId, std::string* err) = 0;
    
    // Estimate verification cost (gas/time)
    virtual uint64_t estimateVerificationCost(const ProofBundle& bundle) const = 0;
    
    // Check if proof system is supported
    virtual bool supportsProofSystem(ProofSystem system) const = 0;
    
    // Get verifier capabilities
    virtual std::vector<ProofSystem> getSupportedSystems() const = 0;
    
    // ========== Cryptographic Utilities ==========
    
    // Verify execution hash chain independently
    virtual bool verifyExecutionHash(const ProofBundle& bundle) const = 0;
    
    // Verify prover signature (if present)
    virtual bool verifyProverSignature(const ProofBundle& bundle) const = 0;
    
    // Check timestamp validity (within acceptable window)
    virtual bool verifyTimestamp(const ProofBundle& bundle,
                                  std::chrono::seconds maxAge) const = 0;
    
    // Check for nonce replay (requires nonce tracking)
    virtual bool verifyNonce(const ProofBundle& bundle) = 0;
    
    // ========== Caching & Performance ==========
    
    // Enable result caching (for repeated verification)
    virtual void enableCache(bool enable, std::size_t maxCacheSize = 1000) = 0;
    
    // Clear verification cache
    virtual void clearCache() = 0;
    
    // Get cache statistics
    struct CacheStats {
        std::size_t hits = 0;
        std::size_t misses = 0;
        std::size_t evictions = 0;
        std::size_t currentSize = 0;
    };
    virtual CacheStats getCacheStats() const = 0;
    
    // ========== Telemetry & Monitoring ==========
    
    struct VerificationMetrics {
        uint64_t totalVerifications = 0;
        uint64_t successfulVerifications = 0;
        uint64_t failedVerifications = 0;
        std::chrono::microseconds avgVerificationTime{0};
        std::chrono::microseconds minVerificationTime{0};
        std::chrono::microseconds maxVerificationTime{0};
        std::size_t totalProofBytes = 0;
    };
    virtual VerificationMetrics getMetrics() const = 0;
    virtual void resetMetrics() = 0;
    
    // Set callback for verification events
    using EventCallback = std::function<void(const std::string& event, 
                                              const VerifyResult& result)>;
    virtual void setEventCallback(EventCallback callback) = 0;
    
    // ========== Security & Auditing ==========
    
    // Enable strict mode (reject proofs with any anomalies)
    virtual void setStrictMode(bool strict) = 0;
    
    // Configure timestamp tolerance
    virtual void setTimestampTolerance(std::chrono::seconds tolerance) = 0;
    
    // Export audit log (for compliance/debugging)
    virtual std::vector<std::string> exportAuditLog() const = 0;
    
    // Get verifier implementation details
    virtual std::string getImplementationInfo() const = 0;
};

// ==================== VERIFIER FACTORY ====================

class VerifierFactory {
public:
    // Create verifier for specific proof system
    static std::unique_ptr<IVerifier> create(ProofSystem system);
    
    // Create verifier from string identifier (for CLI/config)
    static std::unique_ptr<IVerifier> createFromString(const std::string& implId);
    
    // Create verifier with automatic system detection
    static std::unique_ptr<IVerifier> createAuto(const ProofBundle& bundle);
    
    // Get list of available verifier implementations
    static std::vector<ProofSystem> getAvailableVerifiers();
    
    // Check if verifier is available for system
    static bool isVerifierAvailable(ProofSystem system);
    
    // Get proof system name as string
    static std::string getProofSystemName(ProofSystem system);
    
    // Parse proof system from string
    static std::optional<ProofSystem> parseProofSystem(const std::string& name);
    
    // Get recommended verifier for workload
    static ProofSystem getRecommendedVerifier(
        bool requiresFastVerify,
        bool allowTrustedSetup,
        std::size_t maxProofSize);
    
    // Benchmark verifiers and return performance ranking
    static std::vector<ProofSystem> benchmarkVerifiers(
        const std::vector<ProofBundle>& testProofs);
    
    // Register custom verifier implementation
    using VerifierConstructor = std::function<std::unique_ptr<IVerifier>()>;
    static void registerVerifier(ProofSystem system, VerifierConstructor ctor);
};

// ==================== PROOF AGGREGATION ====================

class ProofAggregator {
public:
    // Aggregate multiple proofs into single proof (recursive composition)
    static std::optional<ProofBundle> aggregate(
        const std::vector<ProofBundle>& proofs,
        ProofSystem targetSystem);
    
    // Check if proof system supports aggregation
    static bool supportsAggregation(ProofSystem system);
    
    // Estimate aggregation cost
    static uint64_t estimateAggregationCost(
        const std::vector<ProofBundle>& proofs,
        ProofSystem targetSystem);
};

// ==================== CONVENIENCE UTILITIES ====================

namespace utils {
    // Compute execution hash from components
    std::string computeExecutionHash(
        const std::string& modelHash,
        const std::string& inputHash,
        const std::string& outputHash,
        HashFunction hashFunc = HashFunction::SHA3_256);
    
    // Verify execution hash matches components
    bool verifyExecutionHash(const ProofBundle& bundle);
    
    // Serialize proof bundle to bytes (for network transmission)
    std::vector<uint8_t> serializeProofBundle(const ProofBundle& bundle);
    
    // Deserialize proof bundle from bytes
    std::optional<ProofBundle> deserializeProofBundle(const std::vector<uint8_t>& data);
    
    // Serialize verification key
    std::vector<uint8_t> serializeVerificationKey(const VerificationKey& vk);
    
    // Deserialize verification key
    std::optional<VerificationKey> deserializeVerificationKey(
        const std::vector<uint8_t>& data);
    
    // Convert verification error to string
    std::string verificationErrorToString(VerificationError error);
    
    // Estimate proof size for system (before generation)
    std::size_t estimateProofSize(ProofSystem system, std::size_t publicInputSize);
    
    // Check if proof bundle is well-formed (quick sanity check)
    bool isWellFormedProof(const ProofBundle& bundle);
    
    // Extract public inputs from proof (system-dependent)
    std::optional<std::vector<uint8_t>> extractPublicInputs(
        const ProofBundle& bundle);
    
    // Compare two proofs for equality (cryptographic comparison)
    bool proofsEqual(const ProofBundle& a, const ProofBundle& b);
}

// ==================== NONCE MANAGER (REPLAY PROTECTION) ====================

class NonceManager {
public:
    explicit NonceManager(std::size_t maxTrackedNonces = 100000);
    
    // Check if nonce has been seen before
    bool hasNonce(uint64_t nonce, const std::string& workerId) const;
    
    // Mark nonce as used
    void markNonceUsed(uint64_t nonce, const std::string& workerId);
    
    // Cleanup old nonces (garbage collection)
    void cleanupOldNonces(std::chrono::seconds maxAge);
    
    // Get statistics
    struct NonceStats {
        std::size_t totalTracked = 0;
        std::size_t replayAttempts = 0;
        std::size_t uniqueWorkers = 0;
    };
    NonceStats getStats() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

// ==================== VERIFICATION POOL (ASYNC) ====================

class VerificationPool {
public:
    explicit VerificationPool(std::size_t poolSize, ProofSystem system);
    ~VerificationPool();
    
    // Submit verification task asynchronously
    using ResultCallback = std::function<void(VerifyResult)>;
    void submitAsync(const ProofBundle& bundle,
                     const VerificationKey& vk,
                     ResultCallback callback);
    
    // Verify synchronously (blocking)
    VerifyResult verifySync(const ProofBundle& bundle,
                            const VerificationKey& vk);
    
    // Batch verification (optimized for throughput)
    void submitBatchAsync(const std::vector<ProofBundle>& bundles,
                          const VerificationKey& vk,
                          std::function<void(std::vector<VerifyResult>)> callback);
    
    // Get pool statistics
    struct PoolStats {
        std::size_t totalVerifications = 0;
        std::size_t activeWorkers = 0;
        std::size_t queuedTasks = 0;
        std::chrono::milliseconds avgVerificationTime{0};
        std::size_t failedVerifications = 0;
    };
    PoolStats getStats() const;
    
    // Shutdown pool and wait for pending tasks
    void shutdown();
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

// ==================== INTEGRATION WITH AILEE-CORE ====================

namespace integration {
    // Verify AI inference proof from AmbientAI node
    VerifyResult verifyAIInference(
        const ProofBundle& proof,
        const VerificationKey& modelVK,
        const std::vector<uint8_t>& expectedInput,
        const std::vector<uint8_t>& expectedOutput);
    
    // Verify bandwidth relay proof from NetFlow node
    VerifyResult verifyBandwidthRelay(
        const ProofBundle& proof,
        const VerificationKey& relayVK,
        uint64_t bandwidthClaimed,
        const std::string& nodeId);
    
    // Verify WASM execution proof
    VerifyResult verifyWasmExecution(
        const ProofBundle& proof,
        const VerificationKey& wasmVK,
        const std::string& moduleHash,
        const std::vector<uint8_t>& input,
        const std::vector<uint8_t>& output);
    
    // Generate token reward based on verified proof
    struct RewardCalculation {
        double baseReward = 0.0;
        double performanceMultiplier = 1.0;
        double securityMultiplier = 1.0;
        double finalReward = 0.0;
        std::string reason;
    };
    
    RewardCalculation calculateReward(
        const VerifyResult& verification,
        const ProofBundle& proof,
        double baseRewardRate);
}

} // namespace ailee::zk
