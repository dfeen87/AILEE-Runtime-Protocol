#include "l6/ZKMockBackend.h"
#include <openssl/sha.h>

namespace ailee::l6 {

namespace {

std::string hex_encode(const std::vector<uint8_t>& data) {
    std::string hex_chars = "0123456789abcdef";
    std::string hex;
    hex.reserve(data.size() * 2);
    for (uint8_t b : data) {
        hex.push_back(hex_chars[b >> 4]);
        hex.push_back(hex_chars[b & 0x0F]);
    }
    return hex;
}

std::vector<uint8_t> compute_mock_proof_bytes(
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

    uint8_t digest[32];
    SHA256(input_bytes.data(), input_bytes.size(), digest);

    return std::vector<uint8_t>(digest, digest + 32);
}

} // namespace

ZKProofArtifact ZKMockBackend::generate_proof(
    const ZKBackendConfig& config,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    ZKProofArtifact artifact;

    artifact.proof_bytes = compute_mock_proof_bytes(config.circuit_id, constraints, transcript);

    artifact.metadata.proof_id = hex_encode(artifact.proof_bytes);
    artifact.metadata.constraint_set_id = constraints.id;
    artifact.metadata.transcript_id = transcript.id;
    artifact.metadata.logical_size_bytes = artifact.proof_bytes.size();

    return artifact;
}

bool ZKMockBackend::verify_proof(
    const ZKBackendConfig& config,
    const ZKProofArtifact& artifact,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    std::vector<uint8_t> expected_bytes = compute_mock_proof_bytes(config.circuit_id, constraints, transcript);
    return expected_bytes == artifact.proof_bytes;
}

} // namespace ailee::l6
