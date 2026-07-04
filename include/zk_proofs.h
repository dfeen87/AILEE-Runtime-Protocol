/**
 * zk_proofs.h
 *
 * Zero-Knowledge Proof Module for AILEE / AmbientAI
 *
 * Provides interfaces for generating and verifying ZK proofs for telemetry
 * and federated learning computations. Currently stubbed; ready for libsnark integration.
 *
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ailee::zk {

// Shared SHA-256 utility for deterministic commitments.
std::string sha256Hex(const std::string& input);

struct Proof {
    std::string proofData;    // Serialized zk-proof
    std::string publicInput;  // Hash of computation or telemetry
    std::optional<std::string> anchorCommitmentHash; // Optional L2 anchor hash for verification
    bool verified = false;
    uint64_t timestampMs = 0;
};

class ZKEngine {
public:
    ZKEngine() = default;

    /**
     * Generate a zk-proof for a given computation hash (or telemetry hash)
     * taskId: unique task identifier
     * computationHash: hash of the computation to prove
     */
    Proof generateProof(const std::string& taskId, const std::string& computationHash);

    /**
     * Generate a zk-proof using a real circuit via Rust FFI (Halo2).
     */
    Proof generateHalo2Proof(const std::string& taskId, const std::string& computationHash);

    /**
     * Generate a deterministic zk-proof using a supplied timestamp.
     * This supports reproducible proof validation when the timestamp is known.
     */
    Proof generateProofWithTimestamp(const std::string& taskId,
                                     const std::string& computationHash,
                                     uint64_t timestampMs);

    /**
     * Verify a zk-proof
     * Returns true if the proof is valid
     */
    bool verifyProof(const Proof& proof);

    /**
     * Verify a zk-proof using a real circuit via Rust FFI (Halo2).
     * Returns true if the proof is valid
     */
    bool verifyHalo2Proof(const Proof& proof);

    /**
     * Optional: batch verify multiple proofs efficiently
     */
    bool batchVerify(const std::vector<Proof>& proofs);
};

} // namespace ailee::zk
