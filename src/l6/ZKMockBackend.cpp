#include "l6/ZKMockBackend.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace ailee::l6 {

namespace {

std::string hex_encode(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    return hash;
}

std::vector<uint8_t> compute_mock_proof_bytes(
    const std::string& circuit_id,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    std::vector<uint8_t> data;

    auto constraints_bytes = constraints.to_bytes();
    data.insert(data.end(), constraints_bytes.begin(), constraints_bytes.end());

    auto transcript_bytes = transcript.to_bytes();
    data.insert(data.end(), transcript_bytes.begin(), transcript_bytes.end());

    data.insert(data.end(), circuit_id.begin(), circuit_id.end());

    return sha256(data);
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

    if (artifact.proof_bytes != expected_bytes) {
        return false;
    }

    if (artifact.metadata.constraint_set_id != constraints.id ||
        artifact.metadata.transcript_id != transcript.id) {
        return false;
    }

    return true;
}

} // namespace ailee::l6
