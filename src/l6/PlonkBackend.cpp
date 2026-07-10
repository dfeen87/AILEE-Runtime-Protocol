#include "l6/PlonkBackend.h"
#include <openssl/sha.h>

namespace ailee::l6 {

namespace {

std::string compute_hash_hex(const std::vector<uint8_t>& data) {
    if (data.empty()) return "";
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    std::string hex_chars = "0123456789abcdef";
    std::string hex;
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex.push_back(hex_chars[hash[i] >> 4]);
        hex.push_back(hex_chars[hash[i] & 0x0F]);
    }
    return hex;
}

std::vector<uint8_t> build_deterministic_input(
    const std::string& circuit_id,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    std::vector<uint8_t> input_bytes;
    auto constraints_bytes = constraints.to_bytes();
    input_bytes.insert(input_bytes.end(), constraints_bytes.begin(), constraints_bytes.end());
    auto transcript_bytes = transcript.to_bytes();
    input_bytes.insert(input_bytes.end(), transcript_bytes.begin(), transcript_bytes.end());
    input_bytes.insert(input_bytes.end(), circuit_id.begin(), circuit_id.end());
    return input_bytes;
}

std::vector<uint8_t> compute_dummy_proof_bytes(const std::vector<uint8_t>& input_bytes) {
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(input_bytes.data(), input_bytes.size(), digest);
    return std::vector<uint8_t>(digest, digest + SHA256_DIGEST_LENGTH);
}

} // namespace

ZKProofArtifact PlonkBackend::generate_proof(
    const ZKBackendConfig& config,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    std::vector<uint8_t> input_bytes = build_deterministic_input(
        config.circuit_id, constraints, transcript);

    std::vector<uint8_t> proof_bytes = compute_dummy_proof_bytes(input_bytes);
    std::string proof_id = compute_hash_hex(proof_bytes);

    ZKProofMetadata meta;
    meta.proof_id = proof_id;
    meta.constraint_set_id = constraints.id;
    meta.transcript_id = transcript.id;
    meta.logical_size_bytes = proof_bytes.size();

    ZKProofArtifact artifact;
    artifact.proof_bytes = proof_bytes;
    artifact.metadata = meta;
    return artifact;
}

bool PlonkBackend::verify_proof(
    const ZKBackendConfig& config,
    const ZKProofArtifact& artifact,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    std::vector<uint8_t> input_bytes = build_deterministic_input(
        config.circuit_id, constraints, transcript);

    std::vector<uint8_t> expected_proof_bytes = compute_dummy_proof_bytes(input_bytes);

    return (artifact.proof_bytes == expected_proof_bytes) && (artifact.metadata.proof_id == compute_hash_hex(artifact.proof_bytes));
}

} // namespace ailee::l6
