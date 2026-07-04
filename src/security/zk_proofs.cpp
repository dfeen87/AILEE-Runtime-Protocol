/**
 * zk_proofs.cpp
 *
 * Implements ZK proof generation and verification for AmbientAI.
 * Currently uses placeholder logic; replace with libsnark, bellman, or zk-STARK library calls.
 */

#include "zk_proofs.h"
#include <iostream>
#include <chrono>
#include <functional>
#include <openssl/sha.h>
#include "ailee_rust_ffi.h"

namespace ailee::zk {

std::string sha256Hex(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
    std::string out;
    out.reserve(2 * SHA256_DIGEST_LENGTH);
    static const char* kHex = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        out.push_back(kHex[hash[i] >> 4]);
        out.push_back(kHex[hash[i] & 0x0F]);
    }
    return out;
}

// -----------------------------
// Utility: get current timestamp
// -----------------------------
static uint64_t currentTimestampMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// -----------------------------
// Generate Proof (stub)
// -----------------------------
Proof ZKEngine::generateProof(const std::string& taskId, const std::string& computationHash) {
    Proof proof;
    proof.publicInput = taskId + ":" + computationHash;
    proof.timestampMs = currentTimestampMs();

    // Deterministic proof commitment: hash(publicInput || timestamp)
    proof.proofData = sha256Hex(proof.publicInput + ":" + std::to_string(proof.timestampMs));
    proof.verified = true;

    // Debug
    std::cout << "[ZK] Generated proof for task " << taskId << ": " << proof.proofData << std::endl;

    return proof;
}

Proof ZKEngine::generateHalo2Proof(const std::string& taskId, const std::string& computationHash) {
    Proof proof;
    proof.publicInput = computationHash;
    proof.timestampMs = currentTimestampMs();

    char* proof_out = nullptr;
    int res = generate_halo2_proof_ffi(taskId.c_str(), computationHash.c_str(), &proof_out);

    if (res == 0 && proof_out != nullptr) {
        proof.proofData = std::string(proof_out);
        proof.verified = true;
        free_halo2_proof_ffi(proof_out);
    } else {
        proof.verified = false;
        std::cerr << "Failed to generate Halo2 proof via FFI\n";
    }
    return proof;
}

Proof ZKEngine::generateProofWithTimestamp(const std::string& taskId,
                                           const std::string& computationHash,
                                           uint64_t timestampMs) {
    Proof proof;
    proof.publicInput = taskId + ":" + computationHash;
    proof.timestampMs = timestampMs;
    proof.proofData = sha256Hex(proof.publicInput + ":" + std::to_string(proof.timestampMs));
    proof.verified = true;

    std::cout << "[ZK] Generated proof for task " << taskId
              << " @ " << proof.timestampMs << ": " << proof.proofData << std::endl;

    return proof;
}

// -----------------------------
// Verify Proof (Simulated)
// -----------------------------
bool ZKEngine::verifyProof(const Proof& proof) {
    if (proof.proofData.empty() || proof.publicInput.empty()) return false;

    // TODO: Replace with real ZK-SNARK/STARK verifier (e.g. libsnark::r1cs_gg_ppzksnark_verifier_strong_IC).
    const std::string expected = sha256Hex(proof.publicInput + ":" + std::to_string(proof.timestampMs));
    bool valid = (proof.proofData == expected);

    // Debug
    std::cout << "[ZK] Verified proof: " << (valid ? "SUCCESS" : "FAILURE") << std::endl;

    return valid;
}

bool ZKEngine::verifyHalo2Proof(const Proof& proof) {
    if (proof.proofData.empty() || proof.publicInput.empty()) {
        return false;
    }

    int res = verify_halo2_proof_ffi(proof.proofData.c_str(), proof.publicInput.c_str());
    return res == 1;
}

// -----------------------------
// Batch verification (optional)
// -----------------------------
bool ZKEngine::batchVerify(const std::vector<Proof>& proofs) {
    for (const auto& proof : proofs) {
        if (!verifyProof(proof)) return false;
    }
    return true;
}

} // namespace ailee::zk
