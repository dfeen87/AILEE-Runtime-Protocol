// Licensed under the PolyForm Noncommercial License 1.0.0
// HashProofSystem.h — Production-Grade Hash-Based Verification
// Deterministic, fast proofs for WASM execution verification
// Bridge to full ZK proofs (RISC Zero, Plonky2) in Phase 2

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <mutex>

namespace ailee::security {

/**
 * Hash-based proof structure for execution verification
 * 
 * This is a lightweight alternative to full ZK proofs that provides:
 * - Deterministic verification (same inputs → same outputs)
 * - Merkle-tree based execution traces
 * - Signature-based authentication
 * - Fast generation and verification (<10ms)
 * 
 * NOT cryptographically hiding (execution details are visible)
 * Use as MVP until full ZK integration (RISC Zero) in Phase 2
 * 
 * NOTE: Uses SHA3-256 for hashing with OpenSSL 3.0+ (with SHA256 fallback
 * for older OpenSSL versions)
 */
struct HashProof {
    // Core proof elements
    std::string executionHash;     // SHA3-256(moduleHash + inputHash + outputHash)
    std::string merkleRoot;        // Root of execution trace Merkle tree
    std::vector<std::string> tracePath; // Merkle path for verification
    
    // Metadata
    std::string moduleHash;        // WASM module identifier
    std::string inputHash;         // Hash of input data
    std::string outputHash;        // Hash of output data
    uint64_t instructionCount;     // Total instructions executed
    uint64_t gasConsumed;          // Gas units used
    
    // Authentication
    std::string nodeSignature;     // Ed25519 signature by executing node
    std::string nodePubkey;        // Public key of executor
    
    // Replay protection
    std::chrono::system_clock::time_point timestamp;
    uint64_t nonce;                // Monotonic counter
    
    // Proof type identifier
    std::string proofType = "hash_v1";
    
    // Verification status
    bool verified = false;
};

/**
 * Execution trace for Merkle tree construction
 */
struct ExecutionTrace {
    struct Step {
        std::string opcode;        // WASM instruction
        uint64_t gasUsed;          // Gas consumed at this step
        std::vector<uint8_t> memorySnapshot; // Optional memory state
        uint64_t pc;               // Program counter
    };
    
    std::vector<Step> steps;
    std::string computeMerkleRoot() const;
};

/**
 * Hash-based proof system for execution verification
 * 
 * Usage:
 *   HashProofSystem prover;
 *   auto proof = prover.generateProof(result, trace);
 *   bool valid = HashProofSystem::verifyProof(proof);
 */
class HashProofSystem {
public:
    /**
     * Generate a hash-based proof from execution result
     * 
     * @param moduleHash SHA3-256 of WASM module
     * @param inputHash SHA3-256 of input data
     * @param outputHash SHA3-256 of output data
     * @param trace Optional execution trace for Merkle proof
     * @param nodePrivkey Optional private key for signing
     * @return HashProof structure
     */
    static HashProof generateProof(
        const std::string& moduleHash,
        const std::string& inputHash,
        const std::string& outputHash,
        uint64_t instructionCount,
        uint64_t gasConsumed,
        const std::optional<ExecutionTrace>& trace = std::nullopt,
        const std::optional<std::string>& nodePrivkey = std::nullopt
    );
    
    /**
     * Verify a hash-based proof
     * 
     * Checks:
     * 1. Execution hash = SHA3-256(module + input + output)
     * 2. Merkle root matches trace (if provided)
     * 3. Node signature is valid (if provided)
     * 4. Timestamp is within acceptable range
     * 
     * @param proof Proof to verify
     * @param maxAgeSeconds Maximum acceptable proof age (default: 3600s)
     * @return true if proof is valid
     */
    static bool verifyProof(
        const HashProof& proof,
        uint64_t maxAgeSeconds = 3600
    );
    
    /**
     * Batch verify multiple proofs (more efficient)
     * 
     * @param proofs Vector of proofs to verify
     * @return true if ALL proofs are valid
     */
    static bool batchVerify(const std::vector<HashProof>& proofs);
    
    /**
     * Compute execution hash (deterministic)
     */
    static std::string computeExecutionHash(
        const std::string& moduleHash,
        const std::string& inputHash,
        const std::string& outputHash
    );
    
    /**
     * Sign execution hash with Ed25519 private key
     * 
     * @param executionHash Hash to sign
     * @param privkey Ed25519 private key (64 bytes hex)
     * @return Signature (128 bytes hex)
     */
    static std::string signExecution(
        const std::string& executionHash,
        const std::string& privkey
    );
    
    /**
     * Verify Ed25519 signature
     * 
     * @param executionHash Original hash
     * @param signature Signature to verify
     * @param pubkey Public key (64 bytes hex)
     * @return true if signature is valid
     */
    static bool verifySignature(
        const std::string& executionHash,
        const std::string& signature,
        const std::string& pubkey
    );
    
    /**
     * Compute Merkle root from execution trace
     */
    static std::string computeMerkleRoot(const ExecutionTrace& trace);
    
    /**
     * Generate Merkle path for step index
     */
    static std::vector<std::string> generateMerklePath(
        const ExecutionTrace& trace,
        size_t stepIndex
    );
    
    /**
     * Verify Merkle path
     */
    static bool verifyMerklePath(
        const std::string& leafHash,
        const std::vector<std::string>& path,
        const std::string& root
    );
    
    /**
     * Serialize proof to JSON
     */
    static std::string serializeProof(const HashProof& proof);
    
    /**
     * Deserialize proof from JSON
     */
    static std::optional<HashProof> deserializeProof(const std::string& json);

private:
    // Hash utilities (made friend for ExecutionTrace)
    friend struct ExecutionTrace;
    
    static std::string sha3_256(const std::string& data);
    static std::string sha3_256(const std::vector<uint8_t>& data);
    
    // Merkle tree utilities
    static std::string hashPair(const std::string& left, const std::string& right);
    static std::string hashStep(const ExecutionTrace::Step& step);
    
    // Nonce management
    static uint64_t nextNonce_;
    static std::mutex nonceMutex_;
};

/**
 * Proof aggregator for multi-node consensus
 * 
 * Collects proofs from multiple nodes and determines consensus
 */
class ProofAggregator {
public:
    explicit ProofAggregator(size_t requiredQuorum = 2);
    
    /**
     * Add proof from a node
     */
    void addProof(const HashProof& proof);
    
    /**
     * Check if quorum is reached
     */
    bool hasQuorum() const;
    
    /**
     * Get consensus output hash (most common)
     */
    std::optional<std::string> getConsensusOutput() const;
    
    /**
     * Get all unique output hashes and their counts
     */
    std::vector<std::pair<std::string, size_t>> getOutputDistribution() const;
    
    /**
     * Reset aggregator
     */
    void reset();

private:
    size_t requiredQuorum_;
    std::vector<HashProof> proofs_;
    std::unordered_map<std::string, size_t> outputCounts_;
};

} // namespace ailee::security
