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
// Generate Proof (stub)
// -----------------------------
Proof ZKEngine::generateProof(const std::string& taskId, const std::string& computationHash) {
    Proof proof;
    proof.publicInput = taskId + ":" + computationHash;

    // Deterministic proof commitment without timestamp or randomness.
    // TODO: Replace this mock implementation with an actual ZK library call (e.g., libsnark or Halo2)
    // that outputs a real proof blob.
    proof.proofData = sha256Hex("MOCK_ZK_PROOF:" + proof.publicInput);
    proof.verified = true;

    // Debug
    std::cout << "[ZK] Generated mock proof for task " << taskId << ": " << proof.proofData << std::endl;

    return proof;
}

Proof ZKEngine::generateHalo2Proof(const std::string& taskId, const std::string& computationHash) {
    Proof proof;
    proof.publicInput = computationHash;

    Halo2ProofOutput* proof_out = nullptr;
    int res = generate_halo2_proof_ffi(taskId.c_str(), computationHash.c_str(), &proof_out);

    if (res == 0 && proof_out != nullptr) {
        proof.proofData = std::string(reinterpret_cast<const char*>(proof_out->proof_ptr), proof_out->proof_len);
        proof.commitmentBytes = std::vector<uint8_t>(proof_out->commitment_ptr, proof_out->commitment_ptr + proof_out->commitment_len);
        proof.verified = true;
        free_halo2_proof_ffi(proof_out);
    } else {
        proof.verified = false;
        std::cerr << "Failed to generate Halo2 proof via FFI\n";
    }
    return proof;
}

// -----------------------------
// Verify Proof (Simulated)
// -----------------------------
bool ZKEngine::verifyProof(const Proof& proof) {
    if (proof.proofData.empty() || proof.publicInput.empty()) return false;

    // TODO: Replace with real ZK-SNARK/STARK verifier (e.g. libsnark::r1cs_gg_ppzksnark_verifier_strong_IC).
    const std::string expected = sha256Hex("MOCK_ZK_PROOF:" + proof.publicInput);
    bool valid = (proof.proofData == expected);

    // Debug
    std::cout << "[ZK] Verified proof: " << (valid ? "SUCCESS" : "FAILURE") << std::endl;

    return valid;
}

bool ZKEngine::verifyHalo2Proof(const Proof& proof) {
    if (proof.proofData.empty() || proof.publicInput.empty()) {
        return false;
    }

    int res = verify_halo2_proof_ffi(reinterpret_cast<const unsigned char*>(proof.proofData.data()), proof.proofData.size(), proof.publicInput.c_str());
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
